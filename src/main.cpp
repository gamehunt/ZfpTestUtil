#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>
#include <fstream>

#include "zfp.h"
#include "zfp/constarray1.hpp"

struct app_config{
	int  elements = 256;
	bool simplified_output = false;
	bool verbose = false;
	const char* data_path = "\0";
	zfp_config compression_config = {
		.mode = zfp_mode_fixed_rate,
		.arg = {.rate = -4.0}
	};
};

void usage() {
	std::cout << "zfp_test [-s] [-h] [-n <amount> ] [-d <file>] [-m <expert|rate|precision|accuracy>] [-r <rate>] [-p <precision>] [-t <tolerance>] [-e <maxprec> <minbits> <maxbits> <minexp>]\n"
	<< "-h - Print this message and exit\n"
	<< "-s - Enable simplified output (print data as space-separated stream)\n"
	<< "-v - Enable verbose output (print each element)\n"
	<< "-n - Specify amount of elements in test arrays\n"
	<< "-m - Set compression mode\n"
	<< "-r -p -t -e - Set arguments for compression mode\n" 
	<< "-d - Specify file with test data\n"
	<< "\nDefault prompt (no arguments provided): zfp_test -n 256 -m rate -r 4.0"
	<< std::endl;
}

zfp_mode str2mode(const char* str) {
	if(!strcmp(str, "expert")) {
		return zfp_mode_expert;
	} else if (!strcmp(str, "rate")) {
		return zfp_mode_fixed_rate;
	} else if (!strcmp(str, "precision")) {
		return zfp_mode_fixed_precision;
	} else if (!strcmp(str, "accuracy")) {
		return zfp_mode_fixed_accuracy;
	}

	usage();
	exit(1);
}

const char* mode2str(zfp_mode mode) {
	switch(mode) {
		case zfp_mode_expert:
			return "expert";
		case zfp_mode_fixed_accuracy:
			return "accuracy";
		case zfp_mode_fixed_rate:
			return "rate";
		case zfp_mode_fixed_precision:
			return "precision";
		default:
			return "invalid";
	}
}

#define with_arg \
	if(i == argc - 1) { usage(); exit(1); } \
	next = argv[i + 1];

std::unique_ptr<app_config> parse_config(int argc, const char** argv) {
	std::unique_ptr<app_config> cfg = std::make_unique<app_config>();
	
	for(int i = 1; i < argc; i++) {
		const char* arg  = argv[i];	
		const char* next;

		if(arg[0] == '-') {
			switch(arg[1]) {
				case 'h':
					usage();
					exit(1);
				case 's':
					cfg->simplified_output = true;	
					break;
				case 'v':
					cfg->verbose = true;
					break;
				case 'd': with_arg
					cfg->data_path = next; 
					break;
				case 'n': with_arg
					cfg->elements = atoi(next);
					break;
				case 'm': with_arg
					cfg->compression_config.mode = str2mode(next);
					break;
				case 'r': with_arg
					cfg->compression_config.arg.rate = -atof(next);
					break;
				case 'p': with_arg
					cfg->compression_config.arg.precision = atoi(next);
					break;
				case 't': with_arg
					cfg->compression_config.arg.tolerance = atof(next);
					break;
				case 'e':
					if(i + 4 >= argc) {
						usage();
						exit(1);
					}
					cfg->compression_config.arg.expert.maxprec = atoi(argv[i + 1]);
					cfg->compression_config.arg.expert.maxbits = atoi(argv[i + 2]);
					cfg->compression_config.arg.expert.minbits = atoi(argv[i + 3]);
					cfg->compression_config.arg.expert.minexp  = atoi(argv[i + 4]);
					break;
				default:
					usage();
					exit(1);
			}
		}
	}

	return cfg;
}

int main(int argc, const char** argv) {
	auto cfg = parse_config(argc, argv);


	int n = cfg->elements;

	std::vector<double> data_original(n);

	if(cfg->data_path[0] == '\0') {
		srand(time(0));
		for(int i = 0; i < cfg->elements; i++) {
			data_original[i] = (double) rand() / RAND_MAX * i;
		}
	} else {
		std::ifstream file(cfg->data_path);
		if(!file.is_open()) {
			std::cout << "Failed to open data file.\n";
			exit(2);
		}
		
		double v;
		int i = 0;
		while(file >> v) {
			data_original[i] = v;
			i++;
		}
	}


	zfp::const_array1<double> data_compressed(n, cfg->compression_config, data_original.data());

	double abs_error_sum = 0;

	for(int i = 0; i < n; i++) {
		double orig   = data_original[i];
		double decomp = data_compressed[i];

		double error  = fabs(orig - decomp);

		abs_error_sum += error;

		if(cfg->verbose) {
			if(!cfg->simplified_output) {
				std::cout << "#" << i + 1 
					<< ": Original: "    << orig 
					<< " Decompressed: " << decomp
					<< " Abs: " << error
					<< std::endl;
			} else {
				std::cout <<  i + 1 
					<< " " << orig 
					<< " " << decomp
					<< " " << error;
			}
		}
	}

	int orig = data_original.capacity() * sizeof(data_original[0]);
	int comp = data_compressed.size_bytes();

	if(!cfg->simplified_output) {
		std::cout << std::fixed
		<< "\nElement amount:   " << n
		<< "\nMode:             " << mode2str(cfg->compression_config.mode)
		<< "\nOriginal size:    " << orig << "B"
		<< "\nCompressed size:  " << comp << "B"
		<< "\nCompression rate: " << std::setprecision(2) << (1.0 - (double) comp / orig) * 100 << "%"
		<< "\nMean error:       " << std::setprecision(8) << abs_error_sum / n << "\n";
	} else {
		std::cout << std::fixed
		<< n
		<< " " << mode2str(cfg->compression_config.mode)
		<< " " << orig << "B"
		<< " " << comp << "B"
		<< " " << std::setprecision(2) << (1.0 - (double) comp / orig) * 100 << "%"
		<< " " << std::setprecision(8) << abs_error_sum / n << "\n";
	}

	return 0;
}
