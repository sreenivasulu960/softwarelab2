#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
typedef enum{FALSE,TRUE} boolean;

/* for storing metadata of block*/
typedef struct blockMetaData Header;
 struct blockMetaData{
  size_t size;
  boolean is_free;
  Header* prev;
  Header *next;
};

#define META_SIZE sizeof(Header)
#define SBRK_FAILURE (void*)-1

size_t finalSize = 0;

/*to print colored message i am using ANSI code*/
void red ()
{
  printf("\033[0;31m");
}
void green()
{
  printf("\033[0;32m");
}
void reset ()
{
  printf("\033[0m");
}

void *header = NULL;

/*to print an error message when the sbrk() can't allocate memory*/

void outOfMemory()
{
  red();
  printf("\t[*] Sorry could not allocate requested memory\n");
}

/* if free block is not found then we request space from operating system using sbrk()*/

Header *requestSpaceFrom_OS(Header *lptr,size_t size)
{

  printf("\t[+] Intialising sbrk()....\n");
  Header *block = sbrk(0);
  printf("\t[+] Creating space of size %ld bytes as requested by you...\n",size);
  void *request = sbrk(size+META_SIZE);
  if(request==SBRK_FAILURE)  /*if sbrk() can't allocate memory then it returns (void*)-1 */
  {
    outOfMemory();
    return NULL;
  }
  printf("\t[+] Done.\n");

  if(lptr!=NULL)
  {
    lptr->next=block;
    block->prev=lptr;
    block->next=NULL;
  }
  else
  {
    block->prev = NULL;
    block->next = NULL;
  }
  block->size = size;
  block->is_free = FALSE;

  return block;
}

/*if the free block found is greater than the required size the we split it*/

boolean canSplit(Header* block,size_t size)
{
  boolean ret_val = FALSE;
  if((block->size) - size > 0) ret_val = TRUE;

  return ret_val;
}

Header* split(Header* block, size_t size)
{
  green();
  Header* freePart = (Header*)((block)+size);
  printf("\t[+] Intialising splitting..\n");
  freePart->size = (block->size)-size;
  freePart->is_free = TRUE;
  freePart->next = block->next;
  freePart->prev= block;
  
  block->size = size;
  block->is_free = FALSE;
  block->next=freePart;

  printf("\t[+] Size of new blocks are freeblock-->%ld bytes and allocated block-->%ld bytes\n",freePart->size, block->size);

  return block;

}

/*we search the list the first free block found whose size is greater than or equal to required size*/

Header*  getFreeBlock(Header** end,size_t size)
{
  printf("\t[+] Intialising the search..\n");
  printf("\t[+] Please wait finding the free block of size %ld bytes...\n",size);
  Header* current = header;
  while (current!=NULL)
  {
    *end=current;
    if(current->is_free==TRUE&&current->size>=size) return current;
    current=current->next;
  }


  return NULL;
}

void  *myMalloc(size_t size)
{

  green();
  printf("\t[+] Intialising malloc....\n");
  printf("\t[+] allocating a memory of size %ld bytes.....\n",size );
  if(size<=0)
  {
    printf("\t[+] Done\n");
    return NULL;
  }

  Header *block;
  if(header==NULL)
  {
    printf("\t[+] Since this is your first allocation we are requesting space from operating system please wait..\n");
    block = requestSpaceFrom_OS(NULL,size);
    if(block == NULL)
    {
      outOfMemory();
      return NULL;
    }
    header = block;
    printf("\t[+] Done.\n");
  }
  else
  {
    Header *end = header;
    block = getFreeBlock(&end,size);
    if(block == NULL)
    {
      printf("\t[+] No free block found in the memory so we are requesting space from operating system please wait..\n");
      block = requestSpaceFrom_OS(end,size);
      if(block == NULL)
      {
        outOfMemory();
        return NULL;
      }
      printf("\t[+] Done.\n");
    }
    else
    {
      printf("\t[+] Free block found at %p\n",(block+1) );
      if(canSplit(block, size))
      {
        printf("\t[+] But it is bigger than your requested size\n");
        printf("\t[+] Dont worry we are splitting the block for you..\n");
        block = split(block,size);
        printf("\t[+] Splitting is done.\n");
      }
      block->is_free=FALSE;

    }

  }
  reset();

  return (block+1);  /*block points to the header of that block actual data pointer block+sizeofblock so we return block+1 */
}

/*if there is a free block right to it we merge it and set total size to the current block*/

boolean rightMerge(Header* block)
{
  boolean ret_val = FALSE;
  if(block->next!=NULL&&(block->next)->is_free==TRUE )
  {

    ret_val = TRUE;
  }

  return ret_val;

}

Header* rightMergeBlocks(Header* block)
{
  green();
  printf("\t[+] Intialising  rightmerge...\n");
  Header*next_block = block->next;
  block->size += next_block->size;
  block->next = next_block->next;
  next_block->prev = block->prev;
  finalSize = block->size;

  return block;

}

/*if there is a free block left to it we merge them and set the total size to the left block and return the final size*/

boolean leftMerge(Header* block)
{

  boolean ret_val = FALSE;
  if(block->prev!=NULL&&(block->prev)->is_free==TRUE)
  {
    ret_val=TRUE;
  }

  return ret_val;
}

Header* leftMergeBlocks(Header* block)
{
  green();
  printf("\t[+] Intialising leftmerge..\n");

  Header* prev_block = block->prev;
  prev_block->size += block->size;
  block->prev =  prev_block->prev;
  prev_block->next = block->next;
  finalSize=prev_block->size;

  return block;

}

/*first we check if there is a need to merge then we simply set is_free flag to true we dont actually free it because we can reuse it in future*/

void free(void* ptr)
{
  green();
  printf("\t[+] Intialising free operation..\n");
  printf("\t[-] freeing memory located at %p \n",ptr);
  if(ptr==NULL)
  {
    return;
  }
  Header *block = (Header*)ptr - 1;
  printf("\t[+] Freed memory of size %ld bytes located at %p\n",block->size,ptr);
  if(rightMerge(block))
  {
    printf("\t[+] Looks like we have an free block next to this block\n");
    printf("\t[+] Dont worry we are doing a merge for you sit back and relax..\n");
    block = rightMergeBlocks(block);
    printf("\t[+] Merging is done.\n");
    printf("\t[+] final size of block is %ld bytes\n",finalSize);
  }

  if(leftMerge(block))
  {

    printf("\t[+] Looks like we have an free block before this block\n");
    printf("\t[+] Dont worry we are doing a merge for you sit back and relax..\n");
    block = leftMergeBlocks(block);
    printf("\t[+] Merging is done.\n");
    printf("\t[+] final size of block is %ld bytes\n",finalSize);
  }

  block->is_free = TRUE;
  printf("\t[+] Done.\n");
  reset();

}

void allocate(size_t size)
{

  void* ptr = myMalloc(size);
  green();
  printf("\t[+] Allocated a memory of size %ld bytes at the address %p on heap\n",size,ptr);
  reset();
}

int main()
{
  int option;
  size_t size;
  int* ptr;
  printf("Hi there!\nWelcome to the heap\n");

  while(TRUE)
  {

    printf("1.Allocate memory\n2.Free the allocated memory\n3.To exit");
    printf("\nEnter your option:");
    scanf("%d",&option);

    switch (option)
    {

      case 1:

      {
        printf("\nEnter the size in bytes:");
        scanf("%ld",&size);
        allocate(size);
        break;

      }

      case 2:

      {
        printf("\nEnter the address you wish to free:");
        scanf("%p",&ptr);
        free(ptr);
        break;
      }

      case 3:
      {

        printf("Have a nice day\n");
        exit(0);
      }

    }

}

return 0;

}
