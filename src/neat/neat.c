#include <neat.h>
#include <stdlib.h>
#include <stdio.h>
#include <crtdbg.h>



/****************************************/
const char* _s_sig="XYXYXYXY";
const char* _e_sig="ZWZWZWZW";
struct _MEM_BLOCK{
	struct _MEM_BLOCK* p;
	struct _MEM_BLOCK* n;
	void* ptr;
	size_t sz;
	unsigned int ln;
	const char* fn;
	bool f;
} _mem_head={
	NULL,
	NULL,
	NULL,
	0,
	0,
	NULL,
	false
};
void _dump_mem(void* s,size_t sz){
	printf("Memory Dump Of Address 0x%016llx - 0x%016llx (+ %llu):\n",(unsigned long long int)s,(unsigned long long int)s+sz,sz);
	size_t mx_n=8*(((sz+7)>>3)-1);
	unsigned char mx=1;
	while (mx_n>10){
		mx++;
		mx_n/=10;
	}
	char* f=malloc(mx+20);
	sprintf_s(f,mx+20,"0x%%016llx + %% %ullu: ",mx);
	for (size_t i=0;i<sz;i+=8){
		printf(f,(uintptr_t)s,(uintptr_t)i);
		unsigned char j;
		for (j=0;j<8;j++){
			if (i+j>=sz){
				break;
			}
			printf("%02x",*((unsigned char*)s+i+j));
			printf(" ");
		}
		if (j==0){
			break;
		}
		while (j<8){
			printf("   ");
			j++;
		}
		printf("| ");
		for (j=0;j<8;j++){
			if (i+j>=sz){
				break;
			}
			unsigned char c=*((unsigned char*)s+i+j);
			if (c>0x1f&&c!=0x7f){
				printf("%c  ",(char)c);
			}
			else{
				printf("%02x ",c);
			}
		}
		printf("\n");
	}
	free(f);
}
void _valid_mem(unsigned int ln,const char* fn){
	struct _MEM_BLOCK* n=&_mem_head;
	while (true){
		if (n->ptr!=NULL){
			for (unsigned char i=0;i<8;i++){
				if (*((char*)n->ptr+i)!=*(_s_sig+i)){
					printf("ERROR: Line %u (%s): Address 0x%016llx Allocated at Line %u (%s) has been Corrupted (0x%016llx-%u)!\n",ln,fn,((uint64_t)n->ptr+8),n->ln,n->fn,((uint64_t)n->ptr+8),8-i);
					_dump_mem(n->ptr,n->sz+16);
					raise(SIGABRT);
					return;
				}
			}
			for (unsigned char i=0;i<8;i++){
				if (*((char*)n->ptr+n->sz+i+8)!=*(_e_sig+i)){
					printf("ERROR: Line %u (%s): Address 0x%016llx Allocated at Line %u (%s) has been Corrupted (0x%016llx+%llu+%u)!\n",ln,fn,((uint64_t)n->ptr+8),n->ln,n->fn,((uint64_t)n->ptr+8),n->sz,i+1);
					_dump_mem(n->ptr,n->sz+16);
					raise(SIGABRT);
					return;
				}
			}
			if (n->f==true){
				bool ch=false;
				for (size_t i=0;i<n->sz;i++){
					if (*((unsigned char*)n->ptr+i+8)!=0xdd){
						if (ch==false){
							printf("ERROR: Line %u (%s): Detected Memory Change in Freed Block Allocated at Line %u (%s) (0x%016llx):",ln,fn,n->ln,n->fn,(uint64_t)n->ptr);
							ch=true;
						}
						else{
							printf(";");
						}
						printf(" +%llu (%02x)",i,*((unsigned char*)n->ptr+i+8));
					}
				}
				if (ch==true){
					printf("\n");
					_dump_mem(n->ptr,n->sz+16);
					raise(SIGABRT);
				}
			}
		}
		if (n->n==NULL){
			break;
		}
		n=n->n;
	}
}
void _get_mem_block(const void* ptr,unsigned int ln,const char* fn){
	_valid_mem(ln,fn);
	struct _MEM_BLOCK* n=&_mem_head;
	while ((uint64_t)ptr<(uint64_t)n->ptr||(uint64_t)ptr>(uint64_t)n->ptr+n->sz){
		if (n->n==NULL){
			printf("ERROR: Line %u (%s): Unknown Pointer 0x%016llx!\n",ln,fn,(uint64_t)ptr);
			raise(SIGABRT);
			return;
		}
		n=n->n;
	}
	printf("INFO:  Line %u (%s): Found Memory Block Containing 0x%016llx (+%llu) Allocated at Line %u (%s)!\n",ln,fn,(uint64_t)ptr,(uint64_t)ptr-(uint64_t)n->ptr,n->ln,n->fn);
	_dump_mem(n->ptr,n->sz+16);
}
bool _all_defined(const void* ptr,size_t e_sz,unsigned int ln,const char* fn){
	_valid_mem(ln,fn);
	struct _MEM_BLOCK* n=&_mem_head;
	while (n->ptr!=(unsigned char*)ptr-8){
		if (n->n==NULL){
			printf("ERROR: Line %u (%s): Unknown Pointer 0x%016llx!\n",ln,fn,(uint64_t)ptr);
			raise(SIGABRT);
			return false;
		}
		n=n->n;
	}
	assert((n->sz/e_sz)*e_sz==n->sz);
	bool e=false;
	for (size_t i=0;i<n->sz;i+=e_sz){
		bool f=true;
		for (size_t j=i;j<i+e_sz;j++){
			if (*((unsigned char*)ptr+j)!=0xcd){
				f=false;
				break;
			}
		}
		if (f==true){
			e=true;
			printf("ERROR: Line %u (%s): Found Uninitialised Memory Section in Pointer Allocated at Line %u (%s): 0x%016llx +%llu -> +%llu!\n",ln,fn,n->ln,n->fn,(uint64_t)ptr,i,i+e_sz);
		}
	}
	if (e==true){
		_dump_mem(n->ptr,n->sz+16);
		return false;
	}
	return true;
}
void* _malloc_mem(size_t sz,unsigned int ln,const char* fn){
	_valid_mem(ln,fn);
	if (sz<=0){
		printf("ERROR: Line %u (%s): Negative or Zero Size!\n",ln,fn);
		raise(SIGABRT);
		return NULL;
	}
	struct _MEM_BLOCK* n=&_mem_head;
	while (n->ptr!=NULL){
		if (n->n==NULL){
			n->n=malloc(sizeof(struct _MEM_BLOCK));
			n->n->p=NULL;
			n->n->n=NULL;
			n->n->ptr=NULL;
			n->n->sz=0;
			n->n->ln=0;
			n->n->fn=NULL;
			n->n->f=false;
		}
		n=n->n;
	}
	n->ptr=malloc(sz+16);
	if (n->ptr==NULL){
		printf("ERROR: Line %u (%s): Out of Memory!\n",ln,fn);
		raise(SIGABRT);
		return NULL;
	}
	for (size_t i=0;i<8;i++){
		*((char*)n->ptr+i)=*(_s_sig+i);
		*((char*)n->ptr+sz+i+8)=*(_e_sig+i);
	}
	n->sz=sz;
	n->ln=ln;
	n->fn=fn;
	n->f=false;
	return (void*)((uintptr_t)n->ptr+8);
}
void* _realloc_mem(const void* ptr,size_t sz,unsigned int ln,const char* fn){
	_valid_mem(ln,fn);
	if (ptr==NULL){
		return _malloc_mem(sz,ln,fn);
	}
	assert(sz>0);
	struct _MEM_BLOCK* n=&_mem_head;
	while (n->ptr!=(char*)ptr-8){
		if (n->n==NULL){
			printf("ERROR: Line %u (%s): Reallocating Unknown Pointer! (%p => %llu)\n",ln,fn,ptr,sz);
			raise(SIGABRT);
			break;
		}
		n=n->n;
	}
	if (n->f==true){
		printf("ERROR: Line %u (%s): Reallocating Freed Pointer! (%p => %llu)\n",ln,fn,ptr,sz);
		raise(SIGABRT);
		return NULL;
	}
	n->ptr=realloc(n->ptr,sz+16);
	if (n->ptr==NULL){
		printf("ERROR: Line %u (%s): Out of Memory! (%p => %llu)\n",ln,fn,ptr,sz);
		raise(SIGABRT);
		return NULL;
	}
	for (size_t i=0;i<8;i++){
		*((unsigned char*)n->ptr+i)=*(_s_sig+i);
		*((unsigned char*)n->ptr+sz+i+8)=*(_e_sig+i);
	}
	for (size_t i=n->sz;i<sz;i++){
		*((unsigned char*)n->ptr+i+8)=0xcd;
	}
	n->sz=sz;
	n->ln=ln;
	n->fn=fn;
	return (void*)((uintptr_t)n->ptr+8);
}
void _free_mem(const void* ptr,unsigned int ln,const char* fn){
	_valid_mem(ln,fn);
	struct _MEM_BLOCK* n=&_mem_head;
	while (n->ptr!=(char*)ptr-8){
		if (n->n==NULL){
			printf("ERROR: Line %u (%s): Freeing Unknown Pointer!\n",ln,fn);
			raise(SIGABRT);
			return;
		}
		n=n->n;
	}
	n->f=true;
	for (size_t i=0;i<n->sz;i++){
		*((unsigned char*)n->ptr+i+8)=0xdd;
	}
	// free(n->ptr);
	// n->ptr=NULL;
	// n->sz=0;
	// n->ln=0;
	// n->fn=NULL;
	// if (n->p!=NULL){
	// 	n->p->n=n->n;
	// 	if (n->n!=NULL){
	// 		n->n->p=n->p;
	// 	}
	// 	free(n);
	// }
}
#define _str_i(x) #x
#define _str(x) _str_i(x)
#define _concat(a,b) a##b
#define _arg_c_l(...) _,__VA_ARGS__
#define _arg_c_exp(x) x
#define _arg_c_c(_0,_1,N,...) N
#define _arg_c_exp_va(...) _arg_c_exp(_arg_c_c(__VA_ARGS__,1,0))
#define _arg_c(...)  _arg_c_exp_va(_arg_c_l(__VA_ARGS__))
#define _ret_c(t,...) _concat(_return_,t)(__VA_ARGS__)
#define return(...) _ret_c(_arg_c(__VA_ARGS__),__VA_ARGS__)
#define _return_0() \
	do{ \
		_valid_mem(__LINE__,__func__); \
		return; \
	} while(0)
#define _return_1(r) \
	do{ \
		_valid_mem(__LINE__,__func__); \
		return (r); \
	} while(0)
#undef malloc
#define malloc(sz) _malloc_mem(sz,__LINE__,__func__)
#undef realloc
#define realloc(ptr,sz) _realloc_mem(ptr,sz,__LINE__,__func__)
#undef free
#define free(ptr) _free_mem(ptr,__LINE__,__func__)
#define get_mem_block(ptr) _get_mem_block(ptr,__LINE__,__func__)
#define all_defined(ptr,e_sz) _all_defined(ptr,e_sz,__LINE__,__func__)
/****************************************/



void _print_genome(struct _GENOME* g){
	printf("=======================================================================================================================\nNodes={\n");
	for (uint16_t i=0;i<g->l;i++){
		uint16_t c=0;
		for (uint32_t j=0;j<g->nl;j++){
			if ((g->n+j)->l==i){
				printf("  [L%hu#0x%lx]:",i,(g->n+j)->id);
				for (uint32_t k=0;k<(g->n+j)->outputConnectionsl;k++){
					printf(" 0x%lx,",(*((g->n+j)->outputConnections+k))->id);
				}
				printf("\n");
			}
		}
	}
	printf("};\nConnections={\n");
	for (uint32_t i=0;i<g->cgl;i++){
		printf("  [0x%lx]: %u %f 0x%lx,\n",(g->cg+i)->id,(g->cg+i)->e,(g->cg+i)->w,(g->cg+i)->b);
	}
	printf("};\n=======================================================================================================================\n");
}



int _cmp_genome(const void* a,const void* b){
	return((int)((*((struct _GENOME**)b))->f-(*((struct _GENOME**)a))->f));
}



int _cmp_species(const void* a,const void* b){
	return((int)(((struct _SPECIES*)b)->bf-((struct _SPECIES*)a)->bf));
}



uint32_t _getExcessDisjoint(struct _GENOME* a,struct _GENOME* b){
	uint32_t o=a->cgl+b->cgl;
	for (uint32_t i=0;i<a->cgl;i++){
		for (uint32_t j=0;j<b->cgl;j++){
			if ((a->cg+i)->id==(b->cg+j)->id){
				o-=2;
				break;
			}
		}
	}
	return(o);
}



float _averageWeightDiff(struct _GENOME* a,struct _GENOME* b){
	if (a->cgl==0||b->cgl==0){
		return(0);
	}
	uint32_t m=0;
	float o=0;
	for (uint32_t i=0;i<a->cgl;i++){
		for (uint32_t j=0;j<b->cgl;j++){
			if ((a->cg+i)->id==(b->cg+j)->id){
				m++;
				o+=((a->cg+i)->w>(b->cg+j)->w?(a->cg+i)->w-(b->cg+j)->w:(b->cg+j)->w-(a->cg+i)->w);
				break;
			}
		}
	}
	if (m==0){
		return(100.0f);
	}
	return(o/m);
}



bool _is_same_species(struct _SPECIES* s,struct _GENOME* g){
	uint32_t excessAndDisjoint=_getExcessDisjoint(g,s->rep);
	float averageWeightDiff=_averageWeightDiff(g,s->rep);
	uint32_t largeGenomeNormaliser=(g->cgl<21?1:g->cgl-20);
	float compatibility=(((float)excessAndDisjoint)/largeGenomeNormaliser)+(0.5f*averageWeightDiff);
	return((3>compatibility?true:false));
}



void _clone_genome(struct _GENOME* g,struct _GENOME* o){
	o->i=g->i;
	o->l=g->l;
	o->o=g->o;
	o->nl=g->nl;
	o->n=malloc(g->nl*sizeof(struct _NODE));
	for (uint16_t i=0;i<g->nl;i++){
		(o->n+i)->id=(g->n+i)->id;
		(o->n+i)->l=(g->n+i)->l;
		(o->n+i)->outputConnectionsl=0;
		(o->n+i)->outputConnections=NULL;
		(o->n+i)->_i=0;
		(o->n+i)->_o=0;
	}
	o->cgl=g->cgl;
	o->cg=(g->cgl>0?malloc(g->cgl*sizeof(struct _NODE)):NULL);
	for (uint16_t i=0;i<g->cgl;i++){
		(o->cg+i)->a=(g->cg+i)->a;
		(o->n+(o->cg+i)->a)->outputConnectionsl++;
		(o->n+(o->cg+i)->a)->outputConnections=realloc((o->n+(o->cg+i)->a)->outputConnections,(o->n+(o->cg+i)->a)->outputConnectionsl*sizeof(struct _CONNECTION_GENE*));
		*((o->n+(o->cg+i)->a)->outputConnections+(o->n+(o->cg+i)->a)->outputConnectionsl-1)=o->cg+i;
		(o->cg+i)->b=(g->cg+i)->b;
		(o->cg+i)->w=(g->cg+i)->w;
		(o->cg+i)->e=(g->cg+i)->e;
		(o->cg+i)->id=(g->cg+i)->id;
	}
	o->n_id=g->n_id;
	o->bn=g->bn;
	o->f=0;
	return();
}



struct _GENOME* _rand_genome(struct _SPECIES* s){
	float tf=0;
	for (uint16_t i=0;i<s->l;i++){
		tf+=(*(s->g+i))->f;
	}
	float r=((float)rand())/RAND_MAX*tf;
	float t=0;
	for (uint16_t i=0;i<s->l;i++){
		t+=(*(s->g+i))->f;
		if (t>=r){
			return(*(s->g+i));
		}
	}
	return(*(s->g));
}



uint32_t _get_id(struct _POPULATION* p,struct _GENOME* g,struct _NODE* a,struct _NODE* b){
	for (uint32_t i=0;i<p->_hl;i++){
		if (g->cgl==(p->_h+i)->nl&&a->id==(p->_h+i)->a&&b->id==(p->_h+i)->b){
			bool m=true;
			for (uint32_t j=0;j<g->cgl;j++){
				bool f=false;
				for (uint32_t k=0;k<(p->_h+i)->nl;k++){
					if (*((p->_h+i)->n+k)==(g->cg+j)->id){
						f=true;
						break;
					}
				}
				if (f==false){
					m=false;
					break;
				}
			}
			if (m==true){
				return((p->_h+i)->id);
			}
		}
	}
	p->_hl++;
	p->_h=realloc(p->_h,p->_hl*sizeof(struct _HISTORY));
	(p->_h+p->_hl-1)->a=a->id;
	(p->_h+p->_hl-1)->b=b->id;
	(p->_h+p->_hl-1)->id=p->_nh;
	(p->_h+p->_hl-1)->nl=g->cgl;
	(p->_h+p->_hl-1)->n=malloc(g->cgl*sizeof(uint32_t));
	for (uint32_t i=0;i<g->cgl;i++){
		*((p->_h+p->_hl-1)->n+i)=(g->cg+i)->id;
	}
	p->_nh++;
	return(p->_nh-1);
}



void _add_cg(struct _GENOME* o,struct _POPULATION* p){
	uint32_t* ln=malloc(o->l*sizeof(uint32_t));
	for (uint16_t i=0;i<o->l;i++){
		*(ln+i)=0;
	}
	for (uint32_t i=0;i<o->nl;i++){
		(*(ln+(o->n+i)->l))++;
	}
	uint32_t tc=0;
	for (uint16_t i=0;i<o->l-1;i++){
		uint32_t bl=0;
		for (uint16_t j=i+1;j<o->l;j++){
			bl+=*(ln+j);
		}
		tc+=(*(ln+i))*bl;
	}
	free(ln);
	if (tc==o->cgl){
		printf("Connection Failed: Fully Connected!\n");
		return();
	}
	uint32_t na=(uint32_t)(rand()%o->nl);
	uint32_t nb=(uint32_t)(rand()%o->nl);
	while (true){
		if (na!=nb&&(o->n+na)->id!=(o->n+nb)->id&&(o->n+na)->l!=(o->n+nb)->l){
			bool c=false;
			if ((o->n+na)->l>(o->n+nb)->l){
				for (uint32_t i=0;i<(o->n+nb)->outputConnectionsl;i++){
					if ((*((o->n+nb)->outputConnections+i))->b==(o->n+na)->id){
						c=true;
						break;
					}
				}
			}
			else{
				for (uint32_t i=0;i<(o->n+na)->outputConnectionsl;i++){
					if ((*((o->n+na)->outputConnections+i))->b==(o->n+nb)->id){
						c=true;
						break;
					}
				}
			}
			if (c==false){
				break;
			}
		}
		na=(uint32_t)(rand()%o->nl);
		nb=(uint32_t)(rand()%o->nl);
	}
	if ((o->n+na)->l>(o->n+nb)->l){
		uint32_t tmp=nb;
		nb=na;
		na=tmp;
	}
	o->cgl++;
	o->cg=realloc(o->cg,o->cgl*sizeof(struct _CONNECTION_GENE));
	(o->cg+o->cgl-1)->a=(o->n+na)->id;
	(o->cg+o->cgl-1)->b=(o->n+nb)->id;
	(o->cg+o->cgl-1)->w=((float)rand())/RAND_MAX*2-1;
	while ((o->cg+o->cgl-1)->w==0){
		(o->cg+o->cgl-1)->w=((float)rand())/RAND_MAX*2-1;
	}
	(o->cg+o->cgl-1)->e=true;
	(o->cg+o->cgl-1)->id=_get_id(p,o,o->n+na,o->n+nb);
	(o->n+na)->outputConnectionsl++;
	(o->n+na)->outputConnections=realloc((o->n+na)->outputConnections,(o->n+na)->outputConnectionsl*sizeof(struct _CONNECTION_GENE*));
	*((o->n+na)->outputConnections+(o->n+na)->outputConnectionsl-1)=o->cg+o->cgl-1;
	return();
}



void _create_genome(struct _SPECIES* s,struct _GENOME* o,struct _POPULATION* p){
	struct _GENOME* a=_rand_genome(s);
	if (rand()%4<1){
		_clone_genome(a,o);
	}
	else{
		struct _GENOME* b=_rand_genome(s);
		if (a->f<b->f){
			struct _GENOME* tmp=a;
			a=b;
			b=tmp;
		}
		o->i=a->i;
		o->l=a->l;
		o->o=a->o;
		o->nl=a->nl;
		o->n=malloc(o->nl*sizeof(struct _NODE));
		for (uint32_t i=0;i<o->nl;i++){
			(o->n+i)->id=(a->n+i)->id;
			(o->n+i)->l=(a->n+i)->l;
			(o->n+i)->outputConnectionsl=0;
			(o->n+i)->outputConnections=NULL;
			(o->n+i)->_i=0;
			(o->n+i)->_o=0;
		}
		o->cgl=a->cgl;
		o->cg=(o->cgl>0?malloc(a->cgl*sizeof(struct _CONNECTION_GENE)):NULL);
		o->n_id=a->n_id;
		o->bn=a->bn;
		o->f=0;
		for (uint32_t i=0;i<a->cgl;i++){
			uint32_t j=0;
			bool kf=false;
			for (;j<b->cgl;j++){
				if ((a->cg+i)->id==(b->cg+j)->id){
					kf=true;
					break;
				}
			}
			if (kf==false){
				(o->cg+i)->a=(a->cg+i)->a;
				(o->cg+i)->b=(a->cg+i)->b;
				(o->cg+i)->w=(a->cg+i)->w;
				(o->cg+i)->e=(a->cg+i)->e;
				(o->cg+i)->id=(a->cg+i)->id;
			}
			else{
				(o->cg+i)->e=true;
				if ((a->cg+i)->e==false||(b->cg+j)->e==false){
					if (rand()%4<3){
						(o->cg+i)->e=false;
					}
				}
				if (rand()%2<1){
					(o->cg+i)->a=(a->cg+i)->a;
					(o->cg+i)->b=(a->cg+i)->b;
					(o->cg+i)->w=(a->cg+i)->w;
					(o->cg+i)->e=(a->cg+i)->e;
					(o->cg+i)->id=(a->cg+i)->id;
				}
				else{
					(o->cg+i)->a=(b->cg+i)->a;
					(o->cg+i)->b=(b->cg+i)->b;
					(o->cg+i)->w=(b->cg+i)->w;
					(o->cg+i)->e=(b->cg+i)->e;
					(o->cg+i)->id=(b->cg+i)->id;
				}
			}
			(o->n+(o->cg+i)->a)->outputConnectionsl++;
			(o->n+(o->cg+i)->a)->outputConnections=realloc((o->n+(o->cg+i)->a)->outputConnections,(o->n+(o->cg+i)->a)->outputConnectionsl*sizeof(struct _CONNECTION_GENE*));
			*((o->n+(o->cg+i)->a)->outputConnections+(o->n+(o->cg+i)->a)->outputConnectionsl-1)=o->cg+i;
		}
	}
	if (o->cgl==0){
		_add_cg(o,p);
	}
	if (rand()%5<4){
		for (uint32_t i=0;i<o->cgl;i++){
			if (rand()%10<1){
				(o->cg+i)->w=((float)rand())/RAND_MAX*2-1;
			}
			else{
				(o->cg+i)->w+=((float)rand())/RAND_MAX/50;
				if ((o->cg+i)->w>1){
					(o->cg+i)->w=1;
				}
				if ((o->cg+i)->w<-1){
					(o->cg+i)->w=-1;
				}
			}
		}
	}
	if (rand()%20<1){
		_add_cg(o,p);
	}
	if (rand()%100<1){
		if (o->cgl==0){
			_add_cg(o,p);
			return();
		}
		uint32_t rc=(uint32_t)(rand()%o->cgl);
		while ((o->cg+rc)->a==o->bn&&o->cgl!=1){
			rc=(uint32_t)(rand()%o->cgl);
		}
		(o->cg+rc)->e=false;
		o->nl++;
		o->n=realloc(o->n,o->nl*sizeof(struct _NODE));
		(o->n+o->nl-1)->id=o->n_id;
		(o->n+o->nl-1)->l=(o->n+(o->cg+rc)->a)->l+1;
		(o->n+o->nl-1)->outputConnectionsl=0;
		(o->n+o->nl-1)->outputConnections=NULL;
		(o->n+o->nl-1)->_i=0;
		(o->n+o->nl-1)->_o=0;
		o->n_id++;
		o->cgl+=3;
		o->cg=realloc(o->cg,o->cgl*sizeof(struct _CONNECTION_GENE));
		(o->cg+o->cgl-3)->a=(o->cg+rc)->a;
		(o->cg+o->cgl-3)->b=(o->n+o->nl-1)->id;
		(o->cg+o->cgl-3)->w=1;
		(o->cg+o->cgl-3)->e=true;
		(o->cg+o->cgl-3)->id=_get_id(p,o,(o->n+(o->cg+rc)->a),o->n+o->nl-1);
		(o->cg+o->cgl-2)->a=(o->n+o->nl-1)->id;
		(o->cg+o->cgl-2)->b=(o->cg+rc)->b;
		(o->cg+o->cgl-2)->w=(o->cg+rc)->w;
		(o->cg+o->cgl-2)->e=true;
		(o->cg+o->cgl-2)->id=_get_id(p,o,o->n+o->nl-1,(o->n+(o->cg+rc)->b));
		(o->cg+o->cgl-1)->a=o->bn;
		(o->cg+o->cgl-1)->b=(o->n+o->nl-1)->id;
		(o->cg+o->cgl-1)->w=0;
		(o->cg+o->cgl-1)->e=true;
		(o->cg+o->cgl-1)->id=_get_id(p,o,o->n+o->bn,(o->n+o->nl-1));
		if ((o->n+o->nl-1)->l==(o->n+(o->cg+rc)->b)->l){
			for (uint32_t i=0;i<o->nl-1;i++){
				if ((o->n+i)->l>=(o->n+o->nl-1)->l){
					(o->n+i)->l++;
				}
			}
			o->l++;
		}
		(o->n+(o->cg+rc)->a)->outputConnectionsl++;
		(o->n+(o->cg+rc)->a)->outputConnections=realloc((o->n+(o->cg+rc)->a)->outputConnections,(o->n+(o->cg+rc)->a)->outputConnectionsl*sizeof(struct _CONNECTION_GENE*));
		*((o->n+(o->cg+rc)->a)->outputConnections+(o->n+(o->cg+rc)->a)->outputConnectionsl-1)=o->cg+o->cgl-3;
		(o->n+o->nl-1)->outputConnectionsl++;
		(o->n+o->nl-1)->outputConnections=realloc((o->n+o->nl-1)->outputConnections,(o->n+o->nl-1)->outputConnectionsl*sizeof(struct _CONNECTION_GENE*));
		*((o->n+o->nl-1)->outputConnections+(o->n+o->nl-1)->outputConnectionsl-1)=o->cg+o->cgl-2;
		(o->n+o->bn)->outputConnectionsl++;
		(o->n+o->bn)->outputConnections=realloc((o->n+o->bn)->outputConnections,(o->n+o->bn)->outputConnectionsl*sizeof(struct _CONNECTION_GENE*));
		*((o->n+o->bn)->outputConnections+(o->n+o->bn)->outputConnectionsl-1)=o->cg+o->cgl-1;
	}
	return();
}



Population create_population(uint16_t sz,uint16_t in,uint16_t on,FittingFunction ff){
	Population o=malloc(sizeof(struct _POPULATION));
	o->g=0;
	o->i=in;
	o->o=on;
	o->ff=ff;
	o->_gl=sz;
	o->_g=malloc(sizeof(struct _GENOME)*sz);
	for (uint16_t i=0;i<sz;i++){
		(o->_g+i)->i=in;
		(o->_g+i)->l=2;
		(o->_g+i)->o=on;
		(o->_g+i)->nl=in+on+1;
		(o->_g+i)->n=malloc(sizeof(struct _NODE)*(in+on+1));
		for (uint16_t j=0;j<in;j++){
			((o->_g+i)->n+j)->id=j;
			((o->_g+i)->n+j)->l=0;
			((o->_g+i)->n+j)->outputConnectionsl=0;
			((o->_g+i)->n+j)->outputConnections=NULL;
			((o->_g+i)->n+j)->_i=0;
			((o->_g+i)->n+j)->_o=0;
		}
		((o->_g+i)->n+in)->id=in;
		((o->_g+i)->n+in)->l=0;
		((o->_g+i)->n+in)->outputConnectionsl=0;
		((o->_g+i)->n+in)->outputConnections=NULL;
		((o->_g+i)->n+in)->_i=0;
		((o->_g+i)->n+in)->_o=0;
		for (uint16_t j=in+1;j<in+on+1;j++){
			((o->_g+i)->n+j)->id=j;
			((o->_g+i)->n+j)->l=1;
			((o->_g+i)->n+j)->outputConnectionsl=0;
			((o->_g+i)->n+j)->outputConnections=NULL;
			((o->_g+i)->n+j)->_i=0;
			((o->_g+i)->n+j)->_o=0;
		}
		(o->_g+i)->cgl=0;
		(o->_g+i)->cg=NULL;
		(o->_g+i)->n_id=in+on+1;
		(o->_g+i)->bn=in;
		(o->_g+i)->f=0;
	}
	o->_sl=0;
	o->_s=NULL;
	o->_hl=0;
	o->_h=NULL;
	o->_nh=0;
	return(o);
}



void update_population(Population p){
	for (uint16_t i=0;i<p->_sl;i++){
		(p->_s+i)->l=0;
		assert((p->_s+i)->g!=NULL);
		free((p->_s+i)->g);
		(p->_s+i)->g=NULL;
	}
	for (uint16_t i=0;i<p->_gl;i++){
		struct _NEURAL_NETWORK nn={
			(p->_g+i)->i,
			(p->_g+i)->l,
			(p->_g+i)->o,
			(p->_g+i)->nl,
			(p->_g+i)->n
		};
		(p->_g+i)->f=p->ff(&nn);
		bool f=false;
		for (uint16_t j=0;j<p->_sl;j++){
			if (_is_same_species(p->_s+j,p->_g+i)==true){
				(p->_s+j)->l++;
				(p->_s+j)->g=realloc((p->_s+j)->g,(p->_s+j)->l*sizeof(struct _GENOME*));
				*((p->_s+j)->g+(p->_s+j)->l-1)=p->_g+i;
				f=true;
				break;
			}
		}
		if (f==false){
			p->_sl++;
			p->_s=realloc(p->_s,p->_sl*sizeof(struct _SPECIES));
			(p->_s+p->_sl-1)->l=1;
			(p->_s+p->_sl-1)->g=malloc(sizeof(struct _GENOME*));
			*((p->_s+p->_sl-1)->g)=p->_g+i;
			(p->_s+p->_sl-1)->bf=0;
			(p->_s+p->_sl-1)->st=0;
			(p->_s+p->_sl-1)->rep=malloc(sizeof(struct _GENOME));
			_clone_genome(p->_g+i,(p->_s+p->_sl-1)->rep);
		}
	}
	assert(p->_sl>0);
	for (uint16_t i=0;i<p->_sl;i++){
		assert((p->_s)->l>0);
		if ((p->_s)->l>1){
			qsort((p->_s)->g,(p->_s)->l,sizeof(struct _GENOME*),_cmp_genome);
		}
		if ((*(p->_s)->g)->f>(p->_s)->bf){
			(p->_s)->bf=(*(p->_s)->g)->f;
			(p->_s)->st=0;
		}
		else{
			(p->_s)->st++;
		}
	}
	if (p->_sl>1){
		qsort(p->_s,p->_sl,sizeof(struct _SPECIES),_cmp_species);
	}
	float af=0;
	float* asf=malloc(p->_sl*sizeof(float));
	for (uint16_t i=0;i<p->_sl;i++){
		if ((p->_s+i)->st>=15){
			continue;
		}
		if ((p->_s+i)->l>2){
			(p->_s+i)->l/=2;
			(p->_s+i)->g=realloc((p->_s+i)->g,(p->_s+i)->l*sizeof(struct _GENOME*));
		}
		*(asf+i)=0;
		for (uint16_t j=0;j<(p->_s+i)->l;j++){
			(*((p->_s+i)->g+j))->f/=(p->_s+i)->l;
			*(asf+i)+=(*((p->_s+i)->g+j))->f;
		}
		af+=*(asf+i);
	}
	uint16_t nsl=0;
	struct _SPECIES* ns=NULL;
	for (uint16_t i=0;i<p->_sl;i++){
		if ((p->_s+i)->st>=15||(*(asf+i)/af*p->_gl)<1){
			continue;
		}
		nsl++;
		ns=realloc(ns,nsl*sizeof(struct _SPECIES));
		*(ns+nsl-1)=*(p->_s+i);
	}
	assert(ns!=NULL);
	free(p->_s);
	p->_sl=nsl;
	p->_s=ns;
	printf("Best NN: %f\n",(*(p->_s->g))->f*p->_s->l);
	uint16_t ngi=0;
	struct _GENOME* ng=malloc(p->_gl*sizeof(struct _GENOME));
	for (uint16_t i=0;i<p->_sl;i++){
		assert(ngi<p->_gl);
		_clone_genome(*((p->_s+i)->g),ng+ngi);
		ngi++;
		for (uint16_t j=0;j<((uint16_t)(*(asf+i)/af*p->_gl))-1;j++){
			assert(ngi<p->_gl);
			_create_genome((p->_s+i),ng+ngi,p);
			ngi++;
		}
	}
	free(asf);
	while (ngi<p->_gl){
		_create_genome(p->_s,ng+ngi,p);
		ngi++;
	}
	for (uint16_t i=0;i<p->_gl;i++){
		for (uint16_t j=0;j<(p->_g+i)->nl;j++){
			if (((p->_g+i)->n+j)->outputConnectionsl>0){
				free(((p->_g+i)->n+j)->outputConnections);
			}
		}
		free((p->_g+i)->n);
		if ((p->_g+i)->cg!=NULL){
			free((p->_g+i)->cg);
		}
	}
	free(p->_g);
	p->_g=ng;
	for (uint16_t i=0;i<p->_gl;i++){
		uint32_t nni=0;
		struct _NODE* nn=malloc((p->_g+i)->nl*sizeof(struct _NODE));
		uint32_t* nm=malloc((p->_g+i)->nl*sizeof(uint32_t));
		for (uint16_t j=0;j<(p->_g+i)->l;j++){
			for (uint32_t k=0;k<(p->_g+i)->nl;k++){
				if (((p->_g+i)->n+k)->l==j){
					assert(*(nm+((p->_g+i)->n+k)->id)==0xcdcdcdcd);
					*(nm+((p->_g+i)->n+k)->id)=nni;
					*(nn+nni)=*((p->_g+i)->n+k);
					nni++;
				}
			}
		}
		free((p->_g+i)->n);
		(p->_g+i)->n=nn;
		if (all_defined(nm,sizeof(uint32_t))==false){
			_print_genome(p->_g+i);
			assert(0);
		}
		for (uint32_t j=0;j<(p->_g+i)->cgl;j++){
			((p->_g+i)->cg+j)->a=*(nm+((p->_g+i)->cg+j)->a);
			((p->_g+i)->cg+j)->b=*(nm+((p->_g+i)->cg+j)->b);
		}
	}
	p->g++;
	return();
}



void test_neural_network(NeuralNetwork nn,float* nn_i,float* nn_o){
	for (uint32_t i=0;i<nn->i;i++){
		(nn->n+i)->_o=*(nn_i+i);
	}
	(nn->n+nn->i)->_o=1;
	for (uint32_t i=0;i<nn->nl;i++){
		if ((nn->n+i)->l!=0){
			(nn->n+i)->_o=1/(1+expf(-(nn->n+i)->_i));
		}
		for (uint32_t j=0;j<(nn->n+i)->outputConnectionsl;j++){
			if ((*((nn->n+i)->outputConnections+j))->e==true){
				if ((*((nn->n+i)->outputConnections+j))->b==0xcdcdcdcd){
					get_mem_block((*((nn->n+i)->outputConnections+j)));
					assert(0);
				}
				// _dump_mem((*((nn->n+i)->outputConnections+j)),sizeof(struct _CONNECTION_GENE));
				(nn->n+(*((nn->n+i)->outputConnections+j))->b)->_i+=(*((nn->n+i)->outputConnections+j))->w*(nn->n+i)->_o;
			}
		}
	}
	for (uint32_t i=0;i<nn->o;i++){
		*(nn_o+i)=(nn->n+nn->i+i+1)->_o;
	}
	for (uint32_t i=0;i<nn->nl;i++){
		(nn->n+i)->_i=0;
	}
	return();
}
