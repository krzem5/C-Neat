#include <neat.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>



float fit(NeuralNetwork nn){
	float nn_i[]={0,0};
	float nn_o;
	test_neural_network(nn,nn_i,&nn_o);
	float o=nn_o;
	*nn_i=1;
	test_neural_network(nn,nn_i,&nn_o);
	o+=1-nn_o;
	*(nn_i+1)=1;
	test_neural_network(nn,nn_i,&nn_o);
	o+=nn_o;
	*nn_i=0;
	test_neural_network(nn,nn_i,&nn_o);
	o+=1-nn_o;
	// printf("Error: %f (Fitness: %f/4)\n",o/4,4-o);
	return 4-o;
}



int main(int argc,const char** argv){
	srand(1601797343/*(unsigned int)time(NULL*/);
	Population p=create_population(5,2,1,fit);
	while (true){
		update_population(p);
	}
	return 0;
}
