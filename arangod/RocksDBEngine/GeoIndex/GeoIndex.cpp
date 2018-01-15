////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2017-2018 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Richard Parker
////////////////////////////////////////////////////////////////////////////////

/*   Main GeoIndex routines growing   */

#include "RocksDBEngine/GeoIndex/stones.h"
#include "Basics/priq64.h"
#include "RocksDBEngine/GeoIndex/GeoIndex.h"

namespace arangodb {
namespace geoindex {

extern uint16_t spx[]; // see arangod/RocksDBEngine/GeoIndex/GeoIndexTables.cpp
extern uint16_t hix[];
extern double dix[];

GeoIx * GeoCreate(GeoParm * gp, GeoDetails * gd)
{
    GeoIx * gix;
    GeoLengths gl;
// ignoring GeoDetails entirely!
    gix=(GeoIx*) malloc(sizeof(GeoIx));
    if(gix==NULL) return NULL;
    gl.keylength=14;    // GeoString+payload
    gl.vallength=16;   // doubles lat/long
    gix->st=StonCons(gp,&gl);
    if(gix->st==NULL)
    {
        free(gix);
        return NULL;
    }
    return gix;
}

void GeoDisconnect(GeoIx * gix)
{
    StonDisc(gix->st);
    free(gix);
}

GeoIx * GeoConnect(GeoParm * gp, GeoDetails * gd)
{
    GeoIx * gix;
    GeoLengths gl;
    gix=(GeoIx*) malloc(sizeof(GeoIx));
    gl.keylength=14;    // GeoString+payload
    gl.vallength=16;   // doubles lat/long
    gix->st=StonConnect(gp, &gl);
    return gix;
}
void GeoDrop(GeoParm * gp)
{
    StonDrop(gp);
}

// I strongly recommend you leave the following alone!
// the next two need to be an epsilon too high
#define LATC0 270.00000000000003
#define LONC0 540.00000000000007
// the next two need to be an epsilon too low
#define LATC1 93206.755555555537
#define LONC1 46603.377777777765
// so that all legal values end up 0 . . . 16777215 inclusive

void ToGeoInt(uint64_t Ilt, uint64_t Ilg, uint8_t * gptr)
{
    uint8_t lt0,lt1,lt2,lg0,lg1,lg2;
    uint16_t cb0,cb1,cb2;
    int i1,j,k,gp;   // probably faster than uint16_t
    lt0=Ilt&255;
    lt1=(Ilt>>8)&255;
    lt2=(Ilt>>16)&255;
    lg0=Ilg&255;
    lg1=(Ilg>>8)&255;
    lg2=(Ilg>>16)&255;
    cb0=(spx[lt0]<<1)+spx[lg0];
    cb1=(spx[lt1]<<1)+spx[lg1];
    cb2=(spx[lt2]<<1)+spx[lg2];
    gptr[5]=cb0&255;
    gptr[4]=(cb0>>8)&255;
    gptr[3]=cb1&255;
    gptr[2]=(cb1>>8)&255;
    gptr[1]=cb2&255;
    gptr[0]=(cb2>>8)&255;
    gp=0;
    for(i1=0;i1<6;i1++)
    {
        j=gp+gptr[i1];
        k=hix[j];
        gp=k&0x300;
        gptr[i1]=k&0xFF;
//printf("%3x",gptr[i1]);
    }
//printf("\n");
}

#define BADLONGLAT 17

static long ToGeo(double *lt,   double *lg, uint8_t * geos, long cnt)
{
    long i;

    double lat,lon;
    uint64_t Ilt,Ilg;

    uint8_t * gptr;
    gptr=geos;
    for(i=0;i<cnt;i++)
    {
        lat=lt[i];
        lon=lg[i];
        if( (lat>90.0) || (lat<-90.0) || (lon>180.0) || (lon<-180.0))
            return BADLONGLAT;
        Ilt=((uint64_t)((lat+LATC0)*LATC1))-16777216;
        Ilg=((uint64_t)((lon+LONC0)*LONC1))-16777216;
//printf("%9lu %9lu\n",Ilt,Ilg);
        ToGeoInt(Ilt,Ilg,gptr);
        gptr+=6;
    }
    return 0;
}

int GeoIns(GeoIx * gix, void * trans, GeoPoints * gpt)
{
    uint8_t * geos;
    uint8_t * keyvals;
    uint8_t *pays,*lats,*longs;
    long i,r;
    geos=(uint8_t*)malloc(6*gpt->pointcount);
    r=ToGeo(gpt->latitudes,gpt->longitudes,geos,gpt->pointcount);
    if(r!=0)
    {
        free(geos);
        return r;
    }
    keyvals=(uint8_t*)malloc(30*gpt->pointcount);
    pays=(uint8_t*)gpt->payloads;
    lats=(uint8_t*)gpt->latitudes;
    longs=(uint8_t*)gpt->longitudes;
    for(i=0;i<gpt->pointcount;i++)
    {
        memcpy(keyvals+30*i,geos+6*i,6);
        memcpy(keyvals+30*i+6,pays+8*i,8);
        memcpy(keyvals+30*i+14,lats+8*i,8);
        memcpy(keyvals+30*i+22,longs+8*i,8);
    }
    free(geos);
    r=StonIns(gix->st,trans,keyvals,gpt->pointcount);
    free(keyvals);
    gpt->errorcode=r;
    return r;
}

int GeoDel(GeoIx * gix, void * trans, GeoPoints * gpt)
{
    uint8_t * geos;
    uint8_t * keys;
    uint8_t *pays;
    long i,r;
    geos=(uint8_t*)malloc(6*gpt->pointcount);
    r=ToGeo(gpt->latitudes,gpt->longitudes,geos,gpt->pointcount);
    if(r!=0)
    {
        free(geos);
        return r;
    }
    keys=(uint8_t*)malloc(14*gpt->pointcount);
    pays=(uint8_t*)gpt->payloads;
    for(i=0;i<gpt->pointcount;i++)
    {
        memcpy(keys+14*i,geos+6*i,6);
        memcpy(keys+14*i+6,pays+8*i,8);
    }
    free(geos);
    r=StonDel(gix->st,trans,keys,gpt->pointcount);
    free(keys);
    gpt->errorcode=r;
    return r;
}

// Points handling routines.
// The points pass through (or sometimes skip) four PF (Point Formats)
// PF1 is as a list of 30-byte data items straight out of the
//     database.  GeoString(6) payload(8) latitude(8) longitude(8)
// PF2 is as lists of uint64_t payload double latitude, longitude
//     and also float MDF (Monotonic Distance Function)
// PF3 is a storage area of payload, latitude, longitude
//     held longer term, where the index and MDF are in the heap
// PF4 is the GeoPoints format - again payload, lat, long
//     now ready for delivery to the caller

void PF1PF2(uint8_t * pf1, long pct, uint64_t *py, double *la, double *lg)
{
    long i;
    uint8_t *py8,*la8,*lg8;
    py8=(uint8_t *) py;
    la8=(uint8_t *) la;
    lg8=(uint8_t *) lg;
    for(i=0;i<pct;i++)
    {
        memcpy(py8+8*i,pf1+30*i+ 6,8);
        memcpy(la8+8*i,pf1+30*i+14,8);
        memcpy(lg8+8*i,pf1+30*i+22,8);
    }
}

void DTP(double la, double lg, GeoCursor * cr)
{
    cr->Tarlat=la;
    cr->Tarlong=lg;
    cr->TZ = sin(la * M_PI / 180.0);
    cr->TX = cos(la * M_PI / 180.0) * cos(lg * M_PI / 180.0);
    cr->TY = cos(la * M_PI / 180.0) * sin(lg * M_PI / 180.0);
}

// Monotonic Distance Function is an approximate thing used for
// the heap of points.  It needs to be fast and it does not
// need to be accurate.

// This is the "trig" version.  It is almost certainly too slow
// for a release.  Ideas include Taylor series and/or AVX, but
// let's get it all working first.

// this is the "squared, normalized, mole-distance"

void MDF(GeoCursor *cr, double * la, double * lg, long pct, float * mdf)
{
    long i;
    double x,y,z,X,Y,Z;
    float d;
    X=cr->TX;
    Y=cr->TY;
    Z=cr->TZ;
    for(i=0;i<pct;i++)
    {
        z = sin(la[i] * M_PI / 180.0);
        x = cos(la[i] * M_PI / 180.0) * cos(lg[i] * M_PI / 180.0);
        y = cos(la[i] * M_PI / 180.0) * sin(lg[i] * M_PI / 180.0);
        d=(x-X)*(x-X)+(y-Y)*(y-Y)+(z-Z)*(z-Z);
        mdf[i]=d;       
    }
}

/* Radius of the earth used for distances  */
#define EARTHRADIUS 6371000.0

void GeoMDFToMeters(float * mdf, uint64_t pct, double * meters)
{
    uint64_t i;
    for(i=0;i<pct;i++)
        meters[i]=2.0*EARTHRADIUS*atan(sqrt(mdf[i])/2.0);
}

float GeoMetersToMDF(double meters)
{
    float f;
    f=tan((meters/EARTHRADIUS)/2.0)*2.0;
    return f*f;
}

int NewPf3(GeoCursor *gc)
{
    uint64_t pf3ix,newsiz,i;
    pf3ix=gc->freechain;
    if(pf3ix==0xffffffffffffffff)
    {
        pf3ix=gc->pf3alloc;
        newsiz=pf3ix+pf3ix/2;
        gc->pf3=(PF3*)realloc(gc->pf3,newsiz*sizeof(PF3));
        for(i=pf3ix;i<newsiz-1;i++)
            gc->pf3[i].payfree=i+1;
        gc->pf3[newsiz-1].payfree=0xffffffffffffffff;
        gc->freechain=pf3ix;
        gc->pf3alloc=newsiz;
    }
    gc->freechain=gc->pf3[pf3ix].payfree;
    return pf3ix;
}

void OldPf3(GeoCursor *gc, int pf3ix)
{
    gc->pf3[pf3ix].payfree=gc->freechain;
    gc->freechain=pf3ix;
}

union
{
    uint64_t ui;
    struct
    {
        int ix;
        float mdf;
    };
} mkp;
#define MAXBAB 3
void PF2PF3(GeoCursor *gc, double * la, double * lg, uint64_t * py,
            long pct, float * mdf)
{
    long i;
    int pf3ix;
    uint64_t list[MAXBAB+1];
    list[0]=pct;
    MDF(gc, la, lg, pct, mdf);
    for(i=0;i<pct;i++)
    {
        pf3ix=NewPf3(gc);
        gc->pf3[pf3ix].payfree=py[i];
        gc->pf3[pf3ix].latitude=la[i];
        gc->pf3[pf3ix].longitude=lg[i];
        mkp.ix=pf3ix;
        mkp.mdf=mdf[i];
        list[i+1]=mkp.ui;
    }
    PriQ64Insert(gc->ptheap,list);
}


// GeoIQuery MUST set up the cursor ready for GeoReadCursor
//           MAY do some work on pots and points
//           MUST not return any points
// First version reads the entire database into the heap

#define INITPF3 20
GeoCursor * GeoIQuery(GeoIx * gix, void * trans, GeoQuery * gq)
{
    GeoCursor * gc;
    uint64_t i;
    long j;
    SITR * si;
    uint8_t minkey[14]={0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0};
    uint8_t maxkey[14]={0xff, 0xff, 0xff, 0xff,  0xff, 0xff,
           0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff};
    uint64_t pyb[MAXBAB];
    double lab[MAXBAB],lgb[MAXBAB];
    float mdfb[MAXBAB];
    uint8_t buf[30*MAXBAB];    // space for three records
// make cursor and put data into it
    gc=(GeoCursor*)malloc(sizeof(GeoCursor));
    gc->gix=gix;
    gc->type=gq->type;
    gc->maxmdf=gq->maxmdf;
// including the detailed target point
    DTP(gq->tarlat, gq->tarlong, gc);
// set up the linked list of points
    gc->pf3=(PF3*)malloc(INITPF3*sizeof(PF3));
    gc->pf3alloc=INITPF3;
    for(i=0;i<INITPF3-1;i++)
        gc->pf3[i].payfree=i+1;
    gc->pf3[INITPF3-1].payfree=0xffffffffffffffff;
    gc->freechain=0;
// set up the PriQ64 heap for points
    gc->ptheap=arangodb::basics::PriQ64Cons(INITPF3);

// Now the "Read Whole Database" bit.
// Do the seek
    i=0;
    si=StonSeek(gix->st,trans,minkey);   // start at the beginning
// Read the points 3 at a time
    j=StonRead(gix->st,si,MAXBAB,maxkey,buf);
    while(j>0)
    {
        PF1PF2(buf,j,pyb,lab,lgb);
        PF2PF3(gc,lab,lgb,pyb,j,mdfb);
        j=StonRead(gix->st,si,MAXBAB,maxkey,buf);
    }
    StonDestSitr(si);
    return gc;
}

GeoPoints * GeoReadCursor(GeoCursor * gc, uint64_t pointswanted,
                          float maxmdf1)
{
    long gotpoints;
    uint64_t i,maxinheap;
    uint64_t * ptlist;
    int pf3ix;
    GeoPoints * gp;
    float maxmdf;

    gp=(GeoPoints*)malloc(sizeof(GeoPoints));
    gp->latitudes =(double*)malloc(pointswanted*sizeof(double));
    gp->longitudes=(double*)malloc(pointswanted*sizeof(double));
    gp->payloads=(uint64_t*)malloc(pointswanted*sizeof(uint64_t));
    gp->mdfs=(float*)malloc(pointswanted*sizeof(float));
    maxmdf=maxmdf1;
    if(gc->maxmdf<maxmdf) maxmdf=gc->maxmdf;

// translate maxmdf into maxinheap
    mkp.mdf=maxmdf;
    mkp.ix=-1;
    maxinheap=mkp.ui;

    ptlist=PriQ64Remove(gc->ptheap,maxinheap,pointswanted);
    gotpoints=ptlist[0];
    for(i=0;i<gotpoints;i++)
    {
        mkp.ui=ptlist[i+1];
        pf3ix=mkp.ix;
        gp->latitudes[i]=gc->pf3[pf3ix].latitude;
        gp->longitudes[i]=gc->pf3[pf3ix].longitude;
        gp->payloads[i]=gc->pf3[pf3ix].payfree;
        memcpy(gp->mdfs+i,&(mkp.mdf),4);
        OldPf3(gc,pf3ix);
    }
    gp->pointcount=gotpoints;
    if(gotpoints>0) gp->nextmdf=gp->mdfs[gotpoints-1];
        else        gp->nextmdf=3.0;
// errorcode
    free(ptlist);
    return gp;
}

void GeoDestCursor(GeoCursor * gc)
{
    free(gc->pf3);
    PriQ64Dest(gc->ptheap);
    free(gc);
}

void GeoDestPoints(GeoPoints * gp)
{
    free(gp->latitudes);
    free(gp->longitudes);
    free(gp->payloads);
    free(gp->mdfs);
    free(gp);
}

}
}

