#include "wildcard.hpp"

void wildcard_expand(char * arg) {
	// return if arg does not contain * or ?
	if((!strchr(arg, '*') && !strchr(arg, '?'))
	   || !Command::currentCommand.get_expand()) {
		Command::currentSimpleCommand->insertArgument(arg);
		return;
	}

	bool hidden = false;

	char * a = arg;
  
	for (int i=0;*(a+1);a++, i++) {
		if ((*a == '/' && *(a+1) == '.' && *(a+1) == '*')
			|| (i==0 && *(a)=='.' && *(a+1)=='*')) {
			hidden = true;
		}
	}
 
	glob_t results;
	if(hidden) {
		glob((const char*)arg, GLOB_PERIOD, (int)NULL, &results);
	} else { glob((const char*)arg, GLOB_ERR,(int)NULL, &results); }

	Command::currentCommand.wc_collector.clear();
	Command::currentCommand.wc_collector.shrink_to_fit();
	
 	for(int i=0; i < results.gl_pathc; i++) {
		Command::currentCommand.wc_collector.push_back(results.gl_pathv[i]);
	}  globfree(&results);
}
