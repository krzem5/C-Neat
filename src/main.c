#include <neat.h>
#include <stdlib.h>



float fit(NeuralNetwork nn){
	float i[]={0,0};
	float o[]={0};
	test_neural_network(nn,i,o);
	return 0;
}



int main(int argc,const char** argv){
	Population p=create_population(150,2,1,fit);
	while (true){
		update_population(p);
		break;
	}
	return 0;
}
