#include "wildcard.hpp"

void wildcard_expand(char * arg) {
  // return if arg does not contain * or ?
  if(!strchr(arg, '*') && !strchr(arg, '?')) {
	Command::currentSimpleCommand->insertArgument(arg);
	return;
  }

  char * reg = (char *) calloc(strlen(arg) << 1 + 10, sizeof(char));
  char * a = arg;
  char * r = reg;

  bool beginsExpression = false;
  bool openFirst = false;
  bool hidden = false;

  size_t index = 0;
  size_t dirs = 0;

  std::vector<std::string> directories;
  std::vector<std::string> subs;
  std::vector<std::string> files;
  std::queue<char *> regStrings;

  std::string root ("/");
  std::string curr(".");

  *(r++)= '^';                            // match beginning of line
  for (;*a;a++, index++) {
    switch(*a) {
    case '*':
      *(r++) = '.';
      *(r++) = '*';
      break;
    case '?':
      *(r++) = '.';
      break;
    case '.':
      hidden = true;
      *(r++) = '\\';
      *(r++) = '.';
      break;
    case '/':
      if (!dirs && !index) {
		beginsExpression = true;
		directories.push_back(root);
		dirs++;
      } else {
		*(r++)='$'; *(r++)='\0';         // end regex string pattern
		regStrings.push(strdup(reg));    // copy of pattern and push to queue
		r=reg; *(r++)='^'; dirs++;       // reset regex string pattern
      }
      break;
    default:
      *(r++) = *a;
    }
  } *(r++)='$'; *(r++)='\0';         // match end of line and add null char
  openFirst = true;

  // adds last regex string pattern to queue
  if (!beginsExpression || (beginsExpression && dirs)) {
    regStrings.push(strdup(reg));
    if (!dirs || (!beginsExpression && dirs)) dirs++;
  } if (!beginsExpression) {directories.push_back((char*)curr.c_str());}

  // de-queue patterns and compile
  int size = (int)regStrings.size();
  for (int i=0; i < size; i++) {
    // regex pattern type compilation
    regex_t re; char * str = regStrings.front();
    int result = regcomp(&re, str, REG_EXTENDED|REG_NOSUB);
    if (result) {perror("regcomp"); return;}
    if (dirs) dirs--;

    // open directory and get entries to compare
    DIR * dir;
    for (auto && di : directories) {
      dir = opendir((char*)di.c_str());
      if (dir == NULL) {perror("opendir"); return;}

      // get all directory entries and add to files vector
      for (struct dirent * entry = readdir(dir); entry != NULL; entry = readdir(dir)) {
		std::string e((char*)entry->d_name); files.push_back(e);
      } std::sort(files.begin(), files.end());

      for(auto && f : files) {
		regmatch_t match; std::string e (f);
		if ((!hidden) && (e.front() == '.')) {/* do nothing */}
		else {
		  /* re-compile bc regex cleans up memory -_- */
		  result = regcomp(&re, str, REG_EXTENDED|REG_NOSUB);
		  if (result) {perror("regcomp"); return;}
		  
		  /* match regexp w/ entry */
		  int r = regexec(&re, (char *)e.c_str(), 1, &match, 0);
		  if(r) {/* doesn't match */}
		  else {
			/* just trying to add correct file/dir to arguments */
			if(beginsExpression) {
			  if(openFirst) e = di + e;
			  else {e = di + root + e;}
			  if (!dirs) Command::currentCommand.wc_collector.push_back(e);
			  else {
				DIR * d = opendir((char*)e.c_str());
				if (d) subs.push_back(e);
				closedir(d);
			  }
			} else {
			  if (!openFirst) e = di + root + e;
			  if (!dirs) Command::currentCommand.wc_collector.push_back(e);
			  else {
				DIR * d = opendir((char*)e.c_str());
				if (d) subs.push_back(e);
				closedir(d);}
			}
		  }
		}
      }
      // clear files
      size_t fileSize = files.size();
      for (size_t s = 0; s < fileSize; s++) files.pop_back();
      openFirst = false;
      regfree(&re);
      closedir(dir);
    }
    // remove current regex
    char * top = regStrings.front();
	free(top); regStrings.pop();
	size_t s = directories.size();
    // clear old directories
    for(size_t i = 0; i < s; i++) {directories.pop_back();} s = subs.size();
    // add new directories from subs vector
    for(size_t i = 0; i < s; i++) {directories.push_back(subs.at(i));}
    for(size_t i = 0; i < s; i++) {subs.pop_back();}
  }
}
