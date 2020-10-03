#include <neat.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>



float fit(NeuralNetwork nn){
	float ni[]={0,0};
	float no[]={0};
	test_neural_network(nn,ni,no);
	float o=0-*no;
	*ni=1;
	test_neural_network(nn,ni,no);
	o+=1-*no;
	*(ni+1)=1;
	test_neural_network(nn,ni,no);
	o+=-*no;
	*ni=0;
	test_neural_network(nn,ni,no);
	o+=1-*no;
	printf("ACC: %f\n",o/4);
	return o;
}



int main(int argc,const char** argv){
	srand((unsigned int)time(NULL));
	Population p=create_population(150,2,1,fit);
	while (true){
		update_population(p);
		break;
	}
	return 0;
}
