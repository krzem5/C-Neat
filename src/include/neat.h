#ifndef NEAT_H
#define NEAT_H
#include <stdint.h>
#include <limits.h>
#include <signal.h>



#ifdef NULL
#undef NULL
#endif
#define NULL ((void*)0)
#define bool _Bool
#define true 1
#define false 0
#define assert(x) \
	do{ \
		if (!(x)){ \
			printf("%s:%i (%s): %s: Assertion Failed\n",__FILE__,__LINE__,__func__,#x); \
			raise(SIGABRT); \
		} \
	} while (0)



struct _GENOME;
struct _NODE;
struct _CONNECTION_GENE;



typedef struct _POPULATION* Population;
typedef struct _NEURAL_NETWORK* NeuralNetwork;
typedef float (*FittingFunction)(NeuralNetwork nn);



struct _POPULATION{
	uint32_t g;
	uint16_t i;
	uint16_t o;
	FittingFunction ff;
	uint16_t _gl;
	struct _GENOME* _g;
	uint16_t _sl;
	struct _SPECIES* _s;
	uint32_t _hl;
	struct _HISTORY* _h;
	uint32_t _nh;
};



struct _NEURAL_NETWORK{
	uint16_t i;
	uint16_t l;
	uint16_t o;
	uint32_t nl;
	struct _NODE* n;
};



struct _SPECIES{
	uint16_t l;
	struct _GENOME** g;
	float bf;
	uint32_t st;
	struct _GENOME* rep;
};



struct _GENOME{
	uint16_t i;
	uint16_t l;
	uint16_t o;
	uint32_t nl;
	struct _NODE* n;
	uint32_t cgl;
	struct _CONNECTION_GENE* cg;
	uint32_t n_id;
	uint32_t bn;
	float f;
};



struct _NODE{
	uint32_t id;
	uint16_t l;
	uint32_t outputConnectionsl;
	struct _CONNECTION_GENE** outputConnections;
	float _i;
	float _o;
};



struct _CONNECTION_GENE{
	uint32_t a;
	uint32_t b;
	float w;
	bool e;
	uint32_t id;
};



struct _HISTORY{
	uint32_t a;
	uint32_t b;
	uint32_t id;
	uint32_t nl;
	uint32_t* n;
};



Population create_population(uint16_t sz,uint16_t i,uint16_t o,FittingFunction ff);



void update_population(Population p);



void test_neural_network(NeuralNetwork nn,float* i,float* o);



#endif
