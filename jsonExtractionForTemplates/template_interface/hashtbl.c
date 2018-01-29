/*------------------------------------------------------------------------------

Module:   Hash Table

Purpose:  This is responsible for handling all the hash functions like create, insert, get,
remove functionalities.
Check for thread safety is not handled since only equipment model objects use this interface.
Could be an enhancement going forward.

Filename: hashtbl.c

------------------------------------------------------------------------------*/
#include <hashtbl.h>

/*------------------------------------------------------------------------------
Module:   hashfunc_int method

Purpose:  Handles the hash function for integer keys. Can be called only within this file
since this is static.

Inputs:   key to be searched in the hash
          keyLen - Not used for integer implementation
		  size - Size of the hash. Hash implementation changes based on the size (if power of 2)          

Outputs:  HashIndex - Based on the key, the index where the key is found is returned.
------------------------------------------------------------------------------*/
static UNSIGNED16 hashfunc_int(hashKey *key,UNSIGNED16 keyLen ,hashSize size, hashIndex *idx)
{
   hashIndex  sum;                /* Sum of octets for hash function */

   sum = *((hashIndex *)key);

   /* if size is a power of 2, use shift, else use division */
   if (((size)&(size -1))==0)
      *idx = (sum & (size-1));
   else
      *idx = (sum % size);

	return OK;
}


/*------------------------------------------------------------------------------
Module:   hashfunc_string method

Purpose:  Handles the hash function for string keys. Can be called only within this file
since this is static.
Given a hash key, the algorithm decides which index this key should be part of.
If size of the bucket (hash size) is 1, then there is only 1 bucket, all the keys
will be part of this bucket. In this case index will always be zero.

Inputs:   key to be searched in the hash
          keyLen - Length of the string
		  size - Size of the hash. Hash implementation changes based on the size (if power of 2)          

Outputs:  HashIndex - Based on the key, the index where the key is found is returned..
------------------------------------------------------------------------------*/
static UNSIGNED16 hashfunc_string(hashKey *key,UNSIGNED16 keyLen, hashSize size, hashIndex *idx)
{
   hashIndex  sum = 0;                /* Sum of octets for hash function */
   UNSIGNED16 index = 0;

   if(size == 1)
   {
	   //Only 1 list, index will always be 0
	   *idx = 0;
	   return OK;
   }

   for(index = 0; index < keyLen; index ++)
   {
	  sum = (HASH_RANDOM_INDEXNUMBER*sum)+ key[index];
   }


   /* if size is a power of 2, use shift, else use division */
   if (((size)&(size -1))==0)
      *idx = (sum & (size-1));
   else
      *idx = (sum % size);

	return OK;
}


/*------------------------------------------------------------------------------
Module:   hashfunc_def method

Purpose:  Handles the hash function for default keys. Default key works as an
integer by default. Can be called only within this file since this is static.

Inputs:   key to be searched in the hash
          keyLen - Not used for integer implementation
		  size - Size of the hash. Hash implementation changes based on the size (if power of 2)          

Outputs:  None.
------------------------------------------------------------------------------*/
static UNSIGNED16 hashfunc_def(hashKey *key,UNSIGNED16 keyLen, hashSize size, hashIndex *idx)
{
   hashIndex  sum = 0;                /* Sum of octets for hash function */
   
   while(keyLen--)  
	  sum = sum + (*key++);

   /* if size is a power of 2, use shift, else use division */
   if (((size)&(size -1))==0)
      *idx = (sum & (size-1));
   else
      *idx = (sum % size);

	return OK;
}

/*------------------------------------------------------------------------------
Module:   hashtbl_create method

Purpose:  Creates hash for a given size and hashType

Inputs:   size - Initial bucket size 
		  hashhKeyType - type of hash (support for either string or integer)      

Outputs:  None
------------------------------------------------------------------------------*/
APSHASHTBL * hashtbl_create(hashSize size, UNSIGNED16 hashKeyType)
{
	APSHASHTBL *hashlst = NULL;

	hashlst =  (APSHASHTBL*)OSacquire(sizeof(APSHASHTBL));
	
	if(hashlst == NULL)
		return NULL;

	OSmemset(hashlst,0,sizeof(APSHASHTBL));

	hashlst->nodes=OSacquire(size*sizeof(struct hashEntry_s*));
	if(hashlst->nodes == NULL)
	{
		OSrelease(hashlst);
		return NULL;
	}
	OSmemset(hashlst->nodes,0,size*sizeof(struct hashEntry_s*));
	hashlst->size=size;

   /* initialize hash function for this key type */
   switch (hashKeyType)
   {
      case HASH_TYPE_INT:
		 hashlst->hashFunc= hashfunc_int;
         break;
      case HASH_TYPE_STR:
         hashlst->hashFunc = hashfunc_string;
         break;
	  default:
		hashlst->hashFunc=hashfunc_def;
		break;
   }
   
	return hashlst;
}

void hashtbl_destroy(APSHASHTBL *hashlst)
{
	hashIndex idx;
	struct hashEntry_s *node, *oldnode;
	
	for(idx=0; idx<hashlst->size; ++idx) 
	{
		node = hashlst->nodes[idx];
		while(node) 
		{
			OSrelease(node->key);
			oldnode=node;
			node=node->next;
			OSrelease(oldnode);
		}
	}
	OSrelease(hashlst->nodes);
	OSrelease(hashlst);
	hashlst = NULL;
}

UNSIGNED16 hashtbl_insert(APSHASHTBL *hashlst, hashKey *key, void *data, hashKeyLen keyLen)
{
	struct hashEntry_s *node;
	hashSize hash;
	hashlst->hashFunc(key,keyLen,hashlst->size,&hash);

	node=hashlst->nodes[hash];
	while(node!=NULL) {
	/* If the key already exists, return FAIL*/
		if(!OSmemcmp(node->key, key, node->keyLen)) {
			return HASH_KEY_ALREADYEXISTS_INSERT_ERROR;
		}
		node=node->next;
	}

	if(!(node=OSacquire(sizeof(struct hashEntry_s)))) 
		return HASH_MEMALLOC_INSERT_ERROR;

	node->data=data;
	node->next=hashlst->nodes[hash];
	node->key = OSacquire(keyLen);
	OSmemcpy(node->key,key,keyLen);
	node->keyLen = keyLen;
	hashlst->nodes[hash]=node;

	return OK;
}

UNSIGNED16 hashtbl_remove(APSHASHTBL *hashlst, hashKey *key,hashKeyLen keyLen)
{
	struct hashEntry_s *node, *prevnode=NULL;
	hashSize hash;
	
	hashlst->hashFunc(key,keyLen,hashlst->size,&hash);
	
	//Remove an entry based on the key from the choosen node.
	node=hashlst->nodes[hash];
	while(node) {
		if(!OSmemcmp(node->key, key, node->keyLen)) { //A key is found to be removed.
			OSrelease(node->key); //Release node's key
			node->key = NULL;
			if(prevnode)
				prevnode->next=node->next;
			else 
				hashlst->nodes[hash]=node->next;
			OSrelease(node);
			return OK;
		}
		prevnode=node;
		node=node->next;
	}

	return HASH_KEY_NOTFOUND_REMOVE_ERROR;
}

UNSIGNED16 hashtbl_get(APSHASHTBL *hashlst, hashKey *key, hashKeyLen keyLen, void **data)
{
	struct hashEntry_s *node;
	hashSize hash;
	hashlst->hashFunc(key,keyLen,hashlst->size,&hash);
	*data = NULL;

	node =hashlst->nodes[hash];
	while(node) {
		if(!OSmemcmp(node->key, key, node->keyLen)) 
		{
			*data = node->data;
			return OK;
		}
		node=node->next;
	}
	return HASH_DATA_NOTFOUND_ERROR;
}
