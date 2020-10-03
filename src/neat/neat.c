#include <neat.h>
#include <stdlib.h>
#include <stdio.h>



int _cmp_genome(const void* a,const void* b){
	return (int)((*((struct _GENOME**)b))->f-(*((struct _GENOME**)a))->f);
}



int _cmp_species(const void* a,const void* b){
	return (int)(((struct _SPECIES*)b)->bf-((struct _SPECIES*)a)->bf);
}



uint32_t _getExcessDisjoint(struct _GENOME* a,struct _GENOME* b){
	uint32_t o=a->cgl+b->cgl;
	for (uint32_t i=0;i<a->cgl;i++){
		for (uint32_t j=0;j<b->cgl;j++){
			if ((a->cg+i)->innovationNo==(b->cg+j)->innovationNo){
				o-=2;
				break;
			}
		}
	}
	return o;
}



float _averageWeightDiff(struct _GENOME* a,struct _GENOME* b){
	if (a->cgl==0||b->cgl==0){
		return 0;
	}
	uint32_t m=0;
	float o=0;
	for (uint32_t i=0;i<a->cgl;i++){
		for (uint32_t j=0;j<b->cgl;j++){
			if ((a->cg+i)->innovationNo==(b->cg+j)->innovationNo){
				m++;
				o+=((a->cg+i)->w>(b->cg+j)->w?(a->cg+i)->w-(b->cg+j)->w:(b->cg+j)->w-(a->cg+i)->w);
				break;
			}
		}
	}
	if (m==0){
		return 100.0f;
	}
	return o/m;
}



bool _is_same_species(struct _SPECIES* s,struct _GENOME* g){
	uint32_t excessAndDisjoint=_getExcessDisjoint(g,s->rep);
	float averageWeightDiff=_averageWeightDiff(g,s->rep);
	uint32_t largeGenomeNormaliser=(g->cgl<21?1:g->cgl-20);
	float compatibility=(((float)excessAndDisjoint)/largeGenomeNormaliser)+(0.5f*averageWeightDiff);
	return (3>compatibility?true:false);
}



struct _GENOME* _clone_genome(struct _GENOME* g){
	struct _GENOME* o=malloc(sizeof(struct _GENOME));
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
	}
	o->cgl=g->cgl;
	o->cg=malloc(g->cgl*sizeof(struct _NODE));
	for (uint16_t i=0;i<g->cgl;i++){
		(o->cg+i)->a=(g->cg+i)->a-g->n+o->n;
		(o->cg+i)->a->outputConnectionsl++;
		(o->cg+i)->a->outputConnections=realloc((o->cg+i)->a->outputConnections,(o->cg+i)->a->outputConnectionsl*sizeof(struct _CONNECTION_GENE*));
		*((o->cg+i)->a->outputConnections+(o->cg+i)->a->outputConnectionsl)=o->cg+i;
		(o->cg+i)->b=(g->cg+i)->b-g->n+o->n;
		(o->cg+i)->w=(g->cg+i)->w;
		(o->cg+i)->e=(g->cg+i)->e;
		(o->cg+i)->innovationNo=(g->cg+i)->innovationNo;
	}
	o->n_id=g->n_id;
	o->bn=g->bn;
	return o;
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
		}
		((o->_g+i)->n+in)->id=i;
		((o->_g+i)->n+in)->l=0;
		for (uint16_t j=in+1;j<in+on+1;j++){
			((o->_g+i)->n+j)->id=j;
			((o->_g+i)->n+j)->l=1;
			((o->_g+i)->n+j)->outputConnectionsl=0;
			((o->_g+i)->n+j)->outputConnections=NULL;
		}
		(o->_g+i)->cgl=0;
		(o->_g+i)->cg=NULL;
		(o->_g+i)->n_id=in+on+1;
		(o->_g+i)->bn=i;
		(o->_g+i)->f=-1.23456f/*0*/;////////////////////////////////////////////////////////////
	}
	o->_sl=0;
	o->_s=NULL;
	return o;
}



void update_population(Population p){
	for (uint16_t i=0;i<p->_sl;i++){
		(p->_s+i)->l=0;
		assert((p->_s+i)->g!=NULL);
		free((p->_s+i)->g);
	}
	for (uint16_t i=0;i<p->_gl;i++){
		struct _NEURAL_NETWORK nn={
			p->i,
			(p->_g+i)->l,
			p->o,
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
			(p->_s+p->_sl-1)->bf=0;//////////////////////////////////////////////////////////////
			(p->_s+p->_sl-1)->st=0;
			(p->_s+p->_sl-1)->rep=_clone_genome(p->_g+i);
		}
	}
	/*************************************************/
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
		qsort(p->_s,p->_sl,sizeof(struct _SPECIES*),_cmp_species);
	}
	/*************************************************/
	// cullSpecies();
	// setBestPlayer();
	// killStaleSpecies();
	// killBadSpecies();
	// printBest();
}



void test_neural_network(NeuralNetwork nn,float* i,float* o){
	*o=0;
}
