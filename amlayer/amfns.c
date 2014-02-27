# include <stdio.h>
# include "pf.h"
# include "am.h"
#include <stdbool.h>



/* Creates a secondary idex file called fileName.indexNo */
AM_CreateIndex(fileName,indexNo,attrType,attrLength)
char *fileName;/* Name of indexed file */
int indexNo;/*number of this index for file */
char attrType;/* 'c' for char ,'i' for int ,'f' for float */
int attrLength; /* 4 for 'i' or 'f', 1-255 for 'c' */


{
	char *pageBuf; /* buffer for holding a page */
	char indexfName[AM_MAX_FNAME_LENGTH]; /* String to store the indexed
					 files name with extension           */
	int pageNum; /* page number of the root page(also the first page) */
	int fileDesc; /* file Descriptor */
	int errVal;
	int maxKeys;/* Maximum keys that can be held on one internal page */
	AM_LEAFHEADER head,*header;


	/* Check the parameters */
	if ((attrType != 'c') && (attrType != 'f') && (attrType != 'i'))
	{
	 	AM_Errno = AME_INVALIDATTRTYPE;
	 	return(AME_INVALIDATTRTYPE);
    }
        
	if ((attrLength < 1) || (attrLength > 255))
	{
	 	AM_Errno = AME_INVALIDATTRLENGTH;
	 	return(AME_INVALIDATTRLENGTH);
    }
	
	if (attrLength != 4)
		if (attrType !='c')
			{
			 	AM_Errno = AME_INVALIDATTRLENGTH;
			 	return(AME_INVALIDATTRLENGTH);
            }
	
	header = &head;

	type = attrType;
	// printf("type is %c\n",attrType);

	length = attrLength;
	totalNumberOfPages = 1;
	// header->length = length;

	/* Get the filename with extension and create a paged file by that name*/

	sprintf(indexfName,"%s.%d",fileName,indexNo);
	errVal = PF_CreateFile(indexfName);
	AM_Check;

	/* open the new file */
	fileDesc = PF_OpenFile(indexfName);
	if (fileDesc < 0) 
	{
	   AM_Errno = AME_PF;
	   return(AME_PF);
	}

	/* allocate a new page for the root */
	errVal = PF_AllocPage(fileDesc,&pageNum,&pageBuf);
	AM_Check;
	// printf("sfhinnk\n");
	/* initialise the header */
	header->pageType = 'l';
	header->nextLeafPage = AM_NULL_PAGE;
	header->recIdPtr = PF_PAGE_SIZE;
	header->keyPtr = AM_sl;
	header->freeListPtr = AM_NULL;
	header->numinfreeList = 0;
	header->attrLength = attrLength + AM_si;
	header->numKeys = 0;
	/* the maximum keys in an internal node- has to be even always*/
	maxKeys = (PF_PAGE_SIZE)/(AM_si + attrLength);

	if (( maxKeys % 2) != 0) 
		header->maxKeys = maxKeys - 1;
	else 
		header->maxKeys = maxKeys;
	/* copy the header onto the page */
	bcopy(header,pageBuf,AM_sl);
	
	errVal = PF_UnfixPage(fileDesc,pageNum,TRUE);
	AM_Check;
	
	/* Close the file */
	errVal = PF_CloseFile(fileDesc);
	AM_Check;
	
	/* initialise the root page and the leftmost page numbers */
	AM_RootPageNum = pageNum;
	return(AME_OK);
}


/* Destroys the index fileName.indexNo */
AM_DestroyIndex(fileName,indexNo)
char *fileName;/* name of indexed file */
int indexNo; /* number of this index for file */

{
	char indexfName[AM_MAX_FNAME_LENGTH];
	int errVal;

	sprintf(indexfName,"%s.%d",fileName,indexNo);
	errVal = PF_DestroyFile(indexfName);
	AM_Check;
	return(AME_OK);
}


/* Deletes the recId from the list for value and deletes value if list
becomes empty */
AM_DeleteEntry(fileDesc,attrType,attrLength,value,recId)
int fileDesc; /* file Descriptor */
char attrType; /* 'c' , 'i' or 'f' */
int attrLength; /* 4 for 'i' or 'f' , 1-255 for 'c' */
char *value;/* Value of key whose corr recId is to be deleted */
int recId; /* id of the record to delete */

{
	char *pageBuf;/* buffer to hold the page */
	int pageNum; /* page Number of the page in buffer */
	int index;/* index where key is present */
	int status; /* whether key is in tree or not */
	char *currRecPtr;/* pointer to the current record in the list */
	AM_LEAFHEADER head,*header;/* header of the page */
	int recSize; /* length of key,ptr pair for a leaf */
	int errVal; /* holds the return value of functions called within 
				                            this function */
	int i; /* loop index */

	bool deletedMax = false;
	bool deleted0 = false;


	/* check the parameters */
	if ((attrType != 'c') && (attrType != 'f') && (attrType != 'i'))
	{
	 	AM_Errno = AME_INVALIDATTRTYPE;
	 	return(AME_INVALIDATTRTYPE);
    }

	if (value == NULL) 
	{
	 	AM_Errno = AME_INVALIDVALUE;
	 	return(AME_INVALIDVALUE);
    }

	if (fileDesc < 0) 
	{
	 	AM_Errno = AME_FD;
	 	return(AME_FD);
    }

	/* initialise the header */
	header = &head;
	
	//Changing the defaults to look for valuerecID instead of just value
	// attrType = 'c';
	bcopy(&recId, value + attrLength, AM_si);
	attrLength = attrLength + AM_si;

	/* find the pagenumber and the index of the key to be deleted if it is
	there */
	status = AM_Search(fileDesc,attrType,attrLength,value,&pageNum,
			   &pageBuf,&index);
	// printf("status is %d\n",status);

	recSize = attrLength + AM_si;
	//Check if a0 or aMAX has to be deleted instead
	if(status == AM_NOT_FOUND){
		// recSize = attrLength + AM_si;
		int recId_pseudo = AM_MAX_INT;
		bcopy(&recId_pseudo, value + attrLength - AM_si, AM_si);
		errVal = PF_UnfixPage(fileDesc,pageNum,FALSE);
		status = AM_Search(fileDesc,attrType,attrLength,value,&pageNum,
			   &pageBuf,&index);
		// printf("status2 is %d\n",status);

		if(status == AM_NOT_FOUND){
			//delete a0
			errVal = PF_UnfixPage(fileDesc,pageNum,FALSE);
			int recId_pseudo = 0;
			bcopy(&recId_pseudo, value + attrLength - AM_si, AM_si);

			status = AM_Search(fileDesc,attrType,attrLength,value,&pageNum,
				   &pageBuf,&index);
			deleted0 = true;
		}
		else
		{
			//printf("rID = %d\n",recId);
			int compare = AM_Compare(pageBuf+AM_sl+(index-1)*recSize+attrLength,'i',attrLength-AM_si,&recId);
			// printf("the values compared are %d %d and the result is %d\n",*(pageBuf+AM_sl+(index-1)*recSize+sizeof(char)),
															// recId, compare);
			if(compare!=0)
			{
				//delete aMAX 
				errVal = PF_UnfixPage(fileDesc,pageNum,FALSE);
				int recId_pseudo = 0;
				bcopy(&recId_pseudo, value + attrLength - AM_si, AM_si);

				status = AM_Search(fileDesc,attrType,attrLength,value,&pageNum,
					   &pageBuf,&index);
				deleted0 = true;
			}
			else
			{
				//delete aMAX
				// if(index!=2)
				deletedMax = true;
			}
		}
	}
	
	/* check if return value is an error */
	if (status < 0) 
	{
	 	AM_Errno = status;
	 	return(status);
    }
	
	/* The key is not in the tree */
	if (status == AM_NOT_FOUND) 
	{
	 	AM_Errno = AME_NOTFOUND;
	 	return(AME_NOTFOUND);
    }
	
	bcopy(pageBuf,header,AM_sl);
	// recSize = attrLength + AM_si;
	currRecPtr = pageBuf + AM_sl + (index - 1)*recSize;

	//printf("Del in del %d\n",*(pageBuf+AM_sl+(index-1)*recSize+attrLength));

	/* Delete recId */
	for(i = index; i < (header->numKeys);i++)
		bcopy(pageBuf + AM_sl + i*recSize,pageBuf + AM_sl + 
			(i-1)*recSize,recSize);
	header->numKeys--;
	header->keyPtr = header->keyPtr - recSize;

	// printf("The maxkeys is %d\n",header->maxKeys);

	int zero = 0;
	int max = AM_MAX_INT;
	//Change the sentinels to correct positions
	if(deleted0){	
		bcopy(pageBuf + AM_sl + (index-1)*recSize + attrLength - AM_si,&zero,AM_si);
	}
	else if(deletedMax){
		
		int compare = AM_Compare(pageBuf+AM_sl+(index-2)*recSize+attrLength-AM_si,'i',attrLength-AM_si,&zero);
		if(compare!=0) 
			bcopy(pageBuf + AM_sl + (index-2)*recSize + attrLength - AM_si,&max,AM_si);
	}
	
	/* copy the header onto the buffer */
	bcopy(header,pageBuf,AM_sl);
	
	errVal = PF_UnfixPage(fileDesc,pageNum,TRUE);
	
	/* empty the stack so that it is set for next amlayer call */
	AM_EmptyStack();
	{
		AM_Errno = AME_OK;
		return(AME_OK);
	}
}

/* Inserts a value,recId pair into the tree */
AM_InsertEntry(fileDesc,attrType,attrLength,value,recId)
int fileDesc; /* file Descriptor */
char attrType; /* 'i' or 'c' or 'f' */
int attrLength; /* 4 for 'i' or 'f', 1-255 for 'c' */
char *value; /* value to be inserted */ 
int recId; /* recId to be inserted */

{
	char *pageBuf; /* buffer to hold page */
	int pageNum; /* page number of the page in buffer */
	int index; /* index where key can be found or can be inserted */
	int status; /* whether key is old or new */
	int inserted; /* Whether key has been inserted into the leaf or 
	                      splitting is needed */
	int addtoparent; /* Whether key has to be added to the parent */ 
	int errVal; /* return value of functions within this function */
	char key[AM_MAXATTRLENGTH]; /* holds the attribute to be passed 
						  back to the parent */

	
	/* check the parameters */
	if ((attrType != 'c') && (attrType != 'f') && (attrType != 'i'))
	{
	 	AM_Errno = AME_INVALIDATTRTYPE;
	 	return(AME_INVALIDATTRTYPE);
    }

	if (value == NULL) 
	{
	 	AM_Errno = AME_INVALIDVALUE;
	 	return(AME_INVALIDVALUE);
    }

	if (fileDesc < 0) 
	{
	 	AM_Errno = AME_FD;
	 	return(AME_FD);
    }

 // 	if(recId==168){
	//  	errVal = PF_GetThisPage(fileDesc,0,&pageBuf);
	//  	printf("page type is %c\n",*pageBuf);
	// 	printf("START: The left page number is %d\n",*(pageBuf+AM_sint));
	// 	printf("START: The right page number is %d\n",*(pageBuf+AM_sint+attrLength+AM_si+AM_si));
	// 	// int ee = 1;
	// 	// bcopy(&ee,pageBuf+AM_sint+attrLength+AM_si,AM_si);
	// 	errVal = PF_UnfixPage(fileDesc,pageNum,TRUE);
	// }
    // printf("recID is %d\n", recId);
    status = AM_Search(fileDesc,attrType,attrLength,value,&pageNum,
			   &pageBuf,&index);
    // printf("the status is %d\n",status);
 	
    errVal = PF_UnfixPage(fileDesc,pageNum,FALSE);


	int recId_pseudo = recId;

	//Changing the defaults to look for valuerecID instead of just value
	if(status == AM_NOT_FOUND){
		recId_pseudo = 0;
		// attrType = 'c';
		bcopy(&recId_pseudo, value + attrLength, AM_si);
		attrLength = attrLength + AM_si;
		status = AM_Search(fileDesc,attrType,attrLength,value,&pageNum,
			   &pageBuf,&index);
		 // printf("the status2_notfound is %d\n",status);
    	// if(recId==256){
    		// printf("the status2 is %d\n",status);
    		// printf("The page number obtained for 256 is %d\n",pageNum);
    	// }


	}

	else {
		// printf("recId is %d\n",recId);
		recId_pseudo = AM_MAX_INT;
		// attrType = 'c';
		bcopy(&recId_pseudo, value + attrLength, AM_si);
		attrLength = attrLength + AM_si;
	
	/* Search the leaf for the key */
		status = AM_Search(fileDesc,attrType,attrLength,value,&pageNum,
			   &pageBuf,&index);
		// printf("status2_found is %d\n",status);

		if(status!=AM_NOT_FOUND)
		{
			errVal = PF_UnfixPage(fileDesc,pageNum,FALSE);
			attrLength = attrLength - AM_si;
			recId_pseudo = recId;
			bcopy(&recId_pseudo, value + attrLength, AM_si);
			attrLength = attrLength + AM_si;
			status = AM_Search(fileDesc,attrType,attrLength,value,&pageNum,
			   &pageBuf,&index);
			// printf("status3 is %d\n",status);
			// printf("Page number found is %d\n",pageNum);
		}
	}
	
	/* check if there is an error */
	if (status < 0) 
	{ 
		AM_EmptyStack();
		AM_Errno = status;
		return(status);
	}
	
	/* Insert into leaf the key,recId pair */
	inserted = AM_InsertintoLeaf(pageBuf,attrLength,value,recId,index,
				     status);
	//printf("The inserted value is %d\n", inserted);

    // if(recId==256) printf("the inserted value is %d\n",inserted);


	/* if key has been inserted then done */
	if (inserted == TRUE) 
	{
		// printf("The page number inserted for %d record is %d\n",recId,pageNum);
		errVal = PF_UnfixPage(fileDesc,pageNum,TRUE);
		AM_Check;
		AM_EmptyStack();
		return(AME_OK);
	}
	
	/* check if there is any error */
	if (inserted < 0) 
	{
		AM_EmptyStack();
		AM_Errno = inserted;
		return(inserted);
	}
	
	/* if not inserted then have to split */
	if (inserted == FALSE)
	{
		/* Split the leaf page */
		// printf("RecID is %d\n",recId);
		addtoparent = AM_SplitLeaf(fileDesc,pageBuf,&pageNum,
			     attrLength,recId,value, status,index,key);
		// printf("pagenumber after split is %d\n",pageNum);

		// errVal = PF_GetThisPage(fileDesc,0,&pageBuf);
		// printf("The left page number is %d\n",*(pageBuf+AM_sint));
		// printf("The right page number is %d\n",*(pageBuf+AM_sint+attrLength+AM_si));
		// errVal = PF_UnfixPage(fileDesc,pageNum,TRUE);
		
		/* check for errors */
		if (addtoparent < 0) 
		{
			AM_EmptyStack();
			{
			 	AM_Errno = addtoparent;
			 	return(addtoparent);
            }
		}
		
		/* if key has to be added to the parent */
		if (addtoparent == TRUE)
		{
			errVal = AM_AddtoParent(fileDesc,pageNum,key,attrLength);
			if (errVal < 0)
			{
				AM_EmptyStack();
				AM_Errno = errVal;
				return(errVal);
			}
		}
	}
	AM_EmptyStack();
	// errVal = PF_GetThisPage(fileDesc,0,&pageBuf);
	// printf("END: The left page number is %d\n",*(pageBuf+AM_sint));
	// printf("END: The right page number is %d\n",*(pageBuf+AM_sint+attrLength+AM_si));
	// errVal = PF_UnfixPage(fileDesc,pageNum,TRUE);
	return(AME_OK);
}


/* error messages */
static char *AMerrormsg[] = {
"No error",
"Invalid Attribute Length",
"Key Not Found in Tree",
"PF error",
"Internal error - contact database manager immediately",
"Invalid scan Descriptor",
"Invalid operator to OpenIndexScan",
"Scan Over",
"Scan Table is full",
"Invalid Attribute Type",
"Invalid file Descriptor",
"Invalid value to Delete or Insert Entry"
};


AM_PrintError(s)
char *s;

{
   fprintf(stderr,"%s",s);
   fprintf(stderr,"%s",AMerrormsg[-1*AM_Errno]);
   if (AM_Errno == AME_PF)
      /* print the PF error message */
      PF_PrintError(" ");
   else 
     fprintf(stderr,"\n");
}

