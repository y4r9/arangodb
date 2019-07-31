@RESTSTRUCT{id,api_index_get_struct,string,required,}
The identifier of the index.

@RESTSTRUCT{type,api_index_get_struct,string,required,}
The index type.

@RESTSTRUCT{fields,api_index_get_struct,array,optional,string}
The fields covered by the index.

@RESTSTRUCT{sparse,api_index_get_struct,bool,optional,float}
Flag that shows if the index is sparse.

@RESTSTRUCT{unique,api_index_get_struct,bool,optional,}
Flag that shows if the indexed field values are unique.
