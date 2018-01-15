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

#include "Basics/priq64.h"

/*   PriQ64.c  priority queue of 64-bit integers   */

namespace arangodb {
namespace basics {

uint64_t PriQ64Top(PRIQ64 * pq)
{
    if(pq->nodect==0) return QUEUEEMPTY;
    return pq->heap[0];
}

uint64_t * PriQ64Remove(PRIQ64 * pq, uint64_t biggest,
                        uint64_t maxno)
{
    uint64_t *res;
    uint64_t maxres;
    uint64_t ctleft;
    uint64_t j,ch1,ch2,temp;

    maxres=maxno;
    if(maxres>pq->nodect) maxres=pq->nodect;
    res=(uint64_t*)malloc((maxres+1)*sizeof(uint64_t));
    if(res==NULL) return &(pq->memoryfail);
    res[0]=0;
    ctleft=maxres;
    while(ctleft!=0)
    {
        if(pq->heap[0]>biggest) break;
        res[0]++;
        res[res[0]]=pq->heap[0];
        pq->nodect--;
        pq->heap[0]=pq->heap[pq->nodect];
        j=0;
// sink
        while(1)
        {
            ch1=2*j+1;
            ch2=ch1+1;
            if(ch1>=pq->nodect) break;   // no children at all
            if(ch2>=pq->nodect)  // got exactly 1 child
            {
                if(pq->heap[j]<=pq->heap[ch1]) break;   // heap now OK
                temp=pq->heap[j];
                pq->heap[j]=pq->heap[ch1];
                pq->heap[ch1]=temp;
                break;
            }
//  I'd like this to compile to cmove, but it doesn't!
            if(pq->heap[ch2]<pq->heap[ch1]) ch1=ch2;  // ch1 now smaller
            if(pq->heap[j]<=pq->heap[ch1]) break;   // heap now OK
            temp=pq->heap[j];
            pq->heap[j]=pq->heap[ch1];
            pq->heap[ch1]=temp;
            j=ch1;
        }
        ctleft--;
    }
    return res;
}

 
uint64_t PriQ64Insert(PRIQ64 * pq, uint64_t * list)
{
    uint64_t newmax,expo,i,j,jp,temp;
    uint64_t *newheap;
    newmax=list[0]+pq->nodect;
    if(newmax>pq->alloc)
    {
        expo=(5*pq->alloc)/4;
        if(expo>newmax)newmax=expo;
        newheap=(uint64_t*)realloc(pq->heap,newmax*sizeof(uint64_t));
        if(newheap==NULL)
            return MEMORYFAIL;
        pq->heap=newheap;
        pq->alloc=newmax;
    }
    for(i=1;i<=list[0];i++)
    {
        j=pq->nodect;
        pq->heap[pq->nodect++]=list[i];
//  swim j
        while(j!=0)
        {
            jp=(j-1)/2;   // parent
            if(pq->heap[j]>=pq->heap[jp]) break;
            temp=pq->heap[j];
            pq->heap[j]=pq->heap[jp];
            pq->heap[jp]=temp;
            j=jp;
        }
    }
    return 0;
}

PRIQ64 * PriQ64Cons(uint64_t initialsize)
{
    PRIQ64 * pq;
    pq=(PRIQ64*)malloc(sizeof(PRIQ64));
    if(pq==NULL) return pq;
    pq->heap=(uint64_t*)malloc((initialsize+1)*sizeof(uint64_t)); // zero OK
    if(pq->heap==NULL)
    {
        free(pq);
        return NULL;
    }
    pq->nodect=0;
    pq->alloc=initialsize;
    pq->memoryfail=MEMORYFAIL;    // so we can return a pointer
    return pq;
}


void PriQ64Dest(PRIQ64 * pq)
{
    free(pq->heap);
    free(pq);
    return;
}

void PriQ64Dump(PRIQ64 * pq)
{
    unsigned int i,j;
    printf("Heap has %lu records\n",pq->nodect);
    for(i=0;i<pq->nodect;i++)
    {
        printf("%4d ",i);
        for(j=0;j<8;j++)
            printf(" %2u",(int)(pq->heap[i]>>(56-8*j))&255);
        printf("\n");
    }
}
#ifdef PRIQTEST

/* test program */

void chek(uint64_t * was, uint64_t * shud)
{
    int error,i;
    error=0;
    if(was[0]!=shud[0])error=1;
    if(error==0)
    {
        for(i=1;i<=was[0];i++)
            if(was[i]!=shud[i]) error=1;
    }
    if(error==0) return;
    printf("was %3lu  -  ",was[0]);
    for(i=1;i<=was[0];i++) printf("%3lu ,",was[i]);
    printf("\n");
    printf("shd %3lu  -  ",shud[0]);
    for(i=1;i<=shud[0];i++) printf("%3lu ,",shud[i]);
    printf("\n");
}

int main(int argc, char ** argv)
{
    PRIQ64 * pq;
    uint64_t v1,i,j;
    uint64_t v[110];
    uint64_t t10[]={5,  3,8,7,4,9};   // max 7
    uint64_t r10[]={3,  3,4,7};
    uint64_t t11[]={10,  21,55,31,9,32,17,88,41,77,71};   // 3 max 41
    uint64_t r11a[]={3,  9,17,21};
    uint64_t r11b[]={4,  31,32,41,55};   // 5 more max 55
    uint64_t r11c[]={3,  71,77,88};      // 5 more max 100
    uint64_t r11d[]={0   };              // 5 more max 100
    uint64_t t15[] ={100,  581,541,559,520,592,553,554,577,591,593,
                           981,941,959,920,992,953,954,977,991,993,
                           681,641,659,620,692,653,654,677,691,693,
                           100,101,102,103,104,105,106,107,108,109,
                           381,341,359,320,392,353,354,377,391,393,
                           881,841,859,820,892,853,854,877,891,893,
                           181,141,159,120,192,153,154,177,191,193,
                           281,241,259,220,292,253,254,277,291,293,
                           781,741,759,720,792,753,754,777,791,793,
                           481,441,459,420,492,453,454,477,491,493};
    uint64_t * p;
    
// test 1 - ask for stupidly much memory
    pq=PriQ64Cons(100000000000);
    if(pq!=NULL)
        printf("Test 1 failed\n");
// test 2 - ask for zero - should //  r11c  3,  71,77,88work
    pq=PriQ64Cons(0);
    if(pq==NULL)
        printf("Test 2 failed\n");
// test 3 - free it again
    PriQ64Dest(pq);
// test 4 - put nothing in and ask for top
    pq=PriQ64Cons(5);
    if(pq==NULL)
        printf("Test 4 failed mem\n");
    v1=PriQ64Top(pq);
    if(v1!=QUEUEEMPTY)
        printf("Test 4 failed val\n");
    PriQ64Dest(pq);
// test 5 - put some in
    pq=PriQ64Cons(5);
    v[0]=3;
    v[1]=17;
    v[2]=21;
    v[3]=74;
    v1=PriQ64Insert(pq,v);
    if(v1!=0)
        printf("Test 5 failed return\n");
    v1=PriQ64Top(pq);
    if(v1!=17)
        printf("Test 5 failed top\n");
    PriQ64Dest(pq);
// test 6 - put some in and check sort
    pq=PriQ64Cons(5);
    v[0]=3;
    v[1]=21;
    v[2]=42;
    v[3]=17;
    v1=PriQ64Insert(pq,v);
    if(v1!=0)
        printf("Test 6 failed return\n");
    v1=PriQ64Top(pq);
    if(v1!=17)
        printf("Test 6 failed top\n");
    PriQ64Dest(pq);
// test 7 - test grow and sort
    pq=PriQ64Cons(2);
    v[0]=6;
    v[1]=21;
    v[2]=42;
    v[3]=91;
    v[4]=43;
    v[5]=17;
    v[6]=99;

    v1=PriQ64Insert(pq,v);
    if(v1!=0)
        printf("Test 7 failed return\n");
    v1=PriQ64Top(pq);
    if(v1!=17)
        printf("Test 7 failed top\n");
    PriQ64Dest(pq);
// test 10 - test grow and remove
    pq=PriQ64Cons(2);
    v1=PriQ64Insert(pq,t10);
    if(v1!=0)
        printf("Test 10 failed return\n");
    v1=PriQ64Top(pq);
    if(v1!=3)
        printf("Test 10 failed top\n");
    p=PriQ64Remove(pq,7,10);
    chek(p,r10);
    free(p);
    PriQ64Dest(pq);
// test 11 - test partial remove
//  t11   10,  21,55,31,9,32,17,88,41,77,71};  
//  r11a  3,  9,17,21         3 max 41
//  r11b  4,  31,32,41,55     5 more max 55
//  r11c  3,  71,77,88
//  r11d  0,
    pq=PriQ64Cons(2);
    v1=PriQ64Insert(pq,t11);
    if(v1!=0)
        printf("Test 11 failed return\n");
    v1=PriQ64Top(pq);
    if(v1!=9)
        printf("Test 11 failed top\n");
    p=PriQ64Remove(pq,41,3);
    chek(p,r11a);
    free(p);
    p=PriQ64Remove(pq,55,5);
    chek(p,r11b);
    free(p);
    v1=PriQ64Top(pq);
    if(v1!=71)
        printf("Test 11 failed top\n");
    p=PriQ64Remove(pq,100,5);
    chek(p,r11c);
    free(p);
    p=PriQ64Remove(pq,100,5);
    chek(p,r11d);
    free(p);
    PriQ64Dest(pq);
// test 12 - put in loads and take them out
    pq=PriQ64Cons(100);
    for(i=0;i<100;i++)
    {
        v[0]=100;
        for(j=1;j<=100;j++)
            v[j]=1000*j+i;
        v1=PriQ64Insert(pq,v);
        if(v1!=0)
            printf("Test 12 failed insert\n"); 
    }
    for(j=1;j<=100;j++)
    {
        p=PriQ64Remove(pq,1000000,100);
        if(p[0]!=100)
            printf("Test 12 failed remove\n");
        for(i=0;i<100;i++)
        {
            if(p[i+1]!=1000*j+i)
                printf("Test 12 failed sequence\n");             
        }
        free(p);
    }
    PriQ64Dest(pq);
// test 13 - put in loads one at a time and take them out
    pq=PriQ64Cons(100);
    for(i=0;i<100;i++)
    {
        v[0]=1;
        for(j=1;j<=100;j++)
        {
            v[1]=1000*j+i;
            v1=PriQ64Insert(pq,v);
            if(v1!=0)
                printf("Test 13 failed insert\n");
        }
    }
    for(j=1;j<=100;j++)
    {
        p=PriQ64Remove(pq,1000000,100);
        if(p[0]!=100)
            printf("Test 13 failed remove\n");
        for(i=0;i<100;i++)
        {
            if(p[i+1]!=1000*j+i)
                printf("Test 13 failed sequence\n");             
        }
        free(p);
    }
    PriQ64Dest(pq);
// test 14 - put in loads and take them out one at a time
    pq=PriQ64Cons(100);
    for(i=0;i<100;i++)
    {
        v[0]=100;
        for(j=1;j<=100;j++)
            v[j]=1000*j+i;
        v1=PriQ64Insert(pq,v);
        if(v1!=0)
            printf("Test 14 failed insert\n"); 
    }
    for(j=1;j<=100;j++)
    {
        for(i=0;i<100;i++)
        {
            p=PriQ64Remove(pq,1000000,1);
            if(p[0]!=1)
                printf("Test 14 failed remove\n");
            if(p[1]!=1000*j+i)
                printf("Test 14 failed sequence\n");
            free(p);            
        }
    }
    p=PriQ64Remove(pq,1000000,1);
    chek(p,r11d);
    free(p);
    v1=PriQ64Top(pq);
    if(v1!=QUEUEEMPTY)
        printf("Test 14 failed - not heapempty\n");
    PriQ64Dest(pq);
// test 15 - timing test
    pq=PriQ64Cons(100);
    for(i=0;i<10000000;i++)
    {
        v1=PriQ64Insert(pq,t15);
        if(v1!=0)
            printf("Test 15 failed insert\n");
        p=PriQ64Remove(pq,1000,100);
        free(p);
    }

    v1=PriQ64Top(pq);
    if(v1!=QUEUEEMPTY)
        printf("Test 14 failed - not heapempty\n");
    PriQ64Dest(pq);


    printf("PRIQ64 test completed\n");
    return 0;
}

#endif

}
}
