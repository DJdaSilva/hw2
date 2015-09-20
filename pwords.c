#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#define NUM_THREADS 4

typedef struct dict {
	char *word;
	int count;
	struct dict *next;
} dict_t;

dict_t *First = NULL;
dict_t *wd = NULL;
pthread_mutex_t dictMutex;

FILE *infile = NULL;
pthread_mutex_t fileMutex;

char *make_word(char *word) {
	return strcpy(malloc(strlen(word)+1), word);
}

dict_t *make_dict(char *word) {
	dict_t *nd = (dict_t *) malloc(sizeof(dict_t));
	nd->word = make_word(word);
	nd->count = 1;
	nd->next = NULL;
	return nd;
}

dict_t *insert_word( dict_t *d, char *word ) {
	//Insert word into dict or increment count if already there
	//return pointer to the updated dict

	dict_t *nd;
	dict_t *pd = NULL;	// prior to insertion point 
	dict_t *di = d;		// following insertion point

	// Search down list to find if present or point of insertion
	while(di && ( strcmp(word, di->word ) >= 0) ) { 
		if( strcmp( word, di->word ) == 0 ) { 
			di->count++;			// increment count 
			return d;			// return head 
		}
		pd = di;			// advance ptr pair
		di = di->next;
	}

	nd = make_dict(word);		// not found, make entry
	printf("%s\n", nd->word);
	nd->next = di;			// entry bigger than word or tail 
	if (pd) {
		pd->next = nd;
		return d;			// insert beyond head 
	}

	return nd;
}

void print_dict() {
	dict_t* curr = First;
	while (curr) {
		printf("[%d] %s\n", curr->count, curr->word);
		curr = curr->next;
	}
}

int get_word(char *buf, int n) {
	int inword = 0;
	int c;

	while( (c = fgetc(infile)) != EOF) {
		if (inword && !isalpha(c)) {
			buf[inword] = '\0';	// terminate the word string
			return 1;
		}

		if (isalpha(c)) {
			buf[inword++] = c;
		}
	}
	return 0;			// no more words
}

#define MAXWORD 1024
void *words() {
	
	char wordbuf[MAXWORD];
	int wordIn;
	pthread_mutex_lock(&fileMutex);
	wordIn = get_word(wordbuf, MAXWORD);
	pthread_mutex_unlock(&fileMutex);

	while(wordIn) {
		
		pthread_mutex_lock(&dictMutex);
		wd = insert_word(wd, wordbuf); // add to dict
		pthread_mutex_unlock(&dictMutex);

		pthread_mutex_lock(&fileMutex);
		wordIn = get_word(wordbuf, MAXWORD);
		pthread_mutex_unlock(&fileMutex);
	}

	pthread_mutex_lock(&dictMutex);
	First =  wd;
	pthread_mutex_unlock(&dictMutex);
	pthread_exit(NULL);
}

int main( int argc, char *argv[] ) {
	pthread_t threads[NUM_THREADS];
	int i;
	int res;
	pthread_attr_t threadattr;
	void *status;
	
	if (argc >= 2) {
		infile = fopen (argv[1],"r");
	}

	if( !infile ) {
		printf("Unable to open %s\n",argv[1]);
		exit(EXIT_FAILURE);
	}
	
	//Initialize mutexes
	pthread_mutex_init(&dictMutex, NULL);
	pthread_mutex_init(&fileMutex, NULL);

	//Initialize attribute
	pthread_attr_init(&threadattr);
	pthread_attr_setdetachstate(&threadattr, PTHREAD_CREATE_JOINABLE);

	for (i=0; i<NUM_THREADS; i++) {
		res = pthread_create(&threads[i], &threadattr, words, NULL);

		if (res) {printf("Failed to create thread.\n");}
	}

	for (i=0; i<NUM_THREADS; i++) {
		pthread_join(threads[i], &status);
	}

	print_dict();
	fclose(infile);

	//clean up
	pthread_attr_destroy(&threadattr);
	pthread_mutex_destroy(&dictMutex);
	pthread_mutex_destroy(&fileMutex);
	pthread_exit(NULL);
}
