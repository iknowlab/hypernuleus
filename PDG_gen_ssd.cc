#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <random>

using namespace std;
/* remake 2016.08.19 inoh */

struct reference {
	char ID[16];
	double range,dx,dy,dz;
};

typedef struct {
	/* particle mass */
	double mass;
	/* particle range */	
	double range;
	/* (delta_X')_rms */
	double ssdx;
	/* theta_rms */
	double theta;
} struct_rmsdata;

/* return gaussian distribution random number */
double gaussdistro(double rms){
	double r;//random 0~1
	double ph;//phai 0~2pi
	double t;//inverse function (gaussian squared)
	double x;//result value

	r =(double)rand()/RAND_MAX;
	ph =(double)rand()/(RAND_MAX/(2*M_PI));
	
	/* {f(x)}^(-1) at f(x) = e^{(x^2+y^2)/rms} */
	t = rms * sqrt( -2.0 * log(r) );
	
	/* 2dimensions -> 1dimension */
	x = t*cos(ph);

	return x;
}

int howto(){
	std::cerr << "exe ??RMS_*.dat" << std::endl;
	std::cerr << "G4rms.awk makes ??rms_*.dat" << std::endl;
	std::cerr << "MCS_massrange makes ??rms_*.dat" << std::endl;
	exit(1);
}

int readerr(const char *filename){
	std::cerr << "read file error! (" << filename << ")" << std::endl;
	exit(1);
}

double Read_Param(const char *need){
	std::ifstream prm;
	std::string tag,str;
	double param;
	prm.open("pid.conf");

	if(!prm.fail()){
		while(prm && getline(prm,str)){
			if(str.compare("[ParticleInfo]")==0){
				while(getline(prm,str) && !str.empty()){
					std::stringstream ss(str);
					ss >> tag >> param;
					if(tag.compare(need)==0){
						prm.close();
						return param;
					}//if
				}//while
			}//if
			if(str.compare("[Meas._Error]")==0){
				while(getline(prm,str) && !str.empty()){
					std::stringstream ss(str);
					ss >> tag >> param;
					if(tag.compare(need)==0){
						prm.close();
						return param;
					}//if
				}//while
			}//if
		}//while
	}//if
	else{
		std::cerr << "Imput " << need << " ";
		std::cin >> str;
		param = atof(str.c_str());
		return param;
	}//else
	prm.close();
	
	return param;
}

int main(int argc, char *argv[]){

	/* how to */
	if(argc<2)howto();

	/* read param */
	double step,error;
	step = Read_Param("Step");
	error = Read_Param("MeasErr");

	/* random */
	srand((unsigned)time(NULL));
	mt19937 gen;
	normal_distribution<double> gauss(0,error);

	/* read */
	char linebuffer[1024];
	std::string RF;
	std::string write_line_buffer;
	std::ifstream reading_file[argc-1];/* argc = exe + addfile */
	
	/* rmsfile(datafile,filenum */
	struct_rmsdata rmsdata[128][argc-1];// {range_num,mass_num}
	
	int i, j;

	/* read file */
	for(i=0;i<argc-1;i++){
		reading_file[i].open(argv[i+1]);
		if(reading_file[i].fail())readerr(argv[i+1]);
		j=0;
		std::cout << "read " << argv[i+1] << " ..." << std::endl;

	/* read a row */
		while(reading_file[i] && getline(reading_file[i], RF))
		{
			sscanf(RF.c_str(),"%lf %lf %lf",&rmsdata[j][i].mass,&rmsdata[j][i].range,&rmsdata[j][i].ssdx);
			rmsdata[j][i].theta = rmsdata[j][i].ssdx * (double)sqrt(3.)/step;
			j++;
		}
		reading_file[i].close();
	}

//	std::cerr << "debug mode: " << j << rmsdata[0][0].ssdx << " --> " << rmsdata[j-1][i-1].ssdx << std::endl;
	
	/* calc and write */
	std::ofstream writing_file[j][argc-1];	/* [rmssum] * [argc-1] */
	double rmssum[j][argc-1];	/* calc rms */
	int k;
	int max_j=j;	/* memorize max_j */
	double dx,theta_p;
	char ssdID[64];
	char particleid[16];

	/* make directory */
	char cmd[] = "mkdir ssd";
	std::cout << "command line: mkdir ssd" <<std::endl;
	system(cmd);

	/* write file */

	for(i=0;i<argc-1;i++){
	/* make reference(ssdfile) */
		for(j=0;j<max_j;j++)
		{

/* oOoOoOoOoOoOoOoOo--------------- monte carlo simulation ----------------oOoOoOoOoOoOoOoOo */
			/* write file open */
			sprintf(linebuffer,"./ssd/ssd_%05d_%04d.dat",(int)rmsdata[j][i].range,(int)rmsdata[j][i].mass);
			writing_file[j][i].open(linebuffer);
			if(writing_file[j][i].fail())readerr(linebuffer);
//			std::cerr << "debug mode : file name ..." << linebuffer << std::endl;

			k=0;
			rmssum[j][i]=0.;

			rmsdata[j][i].theta = sqrt(3)*(double)rmsdata[j][i].ssdx/step;//convert theta
			while(k<10000)
			{
				/* gaussian random */
				theta_p = gaussdistro(rmsdata[j][i].theta);//theta plane

				/* delta = (1/2)y_plane = z1*x*theta/sqrt(12) + z2*x*theta/2 */
				dx = (2.*step)*(double)gaussdistro(rmsdata[j][i].theta)/sqrt(12.) 
					+ (2.*step)*(double)theta_p/2.;
				
				/* convert delta_plane from y_plane */
				dx /= 2.0;

				/* add measurement error */
					dx += gauss(gen);

				/* calc rms */
				rmssum[j][i] += dx * dx;

				/* output ssdfile */
				sprintf(ssdID,"%04d%05d",(int)rmsdata[j][i].mass,k);
				writing_file[j][i] << ssdID << " " << rmsdata[j][i].range << " " << dx << " 0.0" << " 0.0 " << theta_p << std::endl;
				k++;
				
			}//while

/* oOoOoOoOoOoOoOoOo--------------- monte carlo simulation ----------------oOoOoOoOoOoOoOoOo */

//			std::cerr << "debug mode: " << rmsdata[j][i].range << rmssum[j][i] << std::endl;
			writing_file[j][i].close();
			
		}//for
	
	std::cout << "\"#" << rmsdata[0][i].mass << "\" was made !" << std::endl;
	}//for

	/* calc rms */
	double rmsout=0.;
	std::ofstream writing_rms;
	writing_rms.open("RMS.dat");
	if(writing_rms.fail())readerr("RMS.dat");

	for(j=0;j<max_j;j++){
		rmsout =0.;

		for(i=0;i<argc-1;i++)
		{
			rmsout += rmssum[j][i];
		}//for

		/* root mean sqare -> rmsout = sqrt( sum(dx*dx) /count(dx) ) */
		rmsout = sqrt( (double)rmsout / ((double)(argc-1)*10000.));

//		std::cout << rmsdata[j][0].range << " " << rmsout << std::endl;
		writing_rms << rmsdata[j][0].range << " " << rmsout << std::endl;

	}//for

	return 0;
}
