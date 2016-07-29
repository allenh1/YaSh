#include "wildcard.hpp"

void wildcard_expand(char * prefix, char * suffix) {
  if (!*suffix && prefix && *prefix)
	{ Command::currentCommand.wc_collector.push_back(std::string(strdup(prefix))); return; }
  else if (!*suffix) return;
  // Get next slash (skipping first, if necessary)
  char * slash = strchr((*suffix == '/') ? suffix + 1: suffix, '/');
  char * next  = (char*) calloc(MAXLEN + 1, sizeof(char));
  // This line is magic.
  for (char * temp = next; *suffix && suffix != slash; *(temp++) = *(suffix++));

  // Expand the wildcard
  char * nextPrefix = (char*) calloc(MAXLEN + 1, sizeof(char));
  if (!(strchr(next, '*') || strchr(next, '?'))) {
	// No more wildcards.
	if (!(prefix && *prefix)) sprintf(nextPrefix, "%s", next);
	else sprintf(nextPrefix, "%s%s", prefix, next);
	wildcard_expand(nextPrefix, suffix);
	free(nextPrefix); free(next); return;
  }

  // Wildcards were found!
  // Convert to regex.
  char * rgx = (char*) calloc(2 * strlen(next) + 3, sizeof(char));
  char * trx = rgx;

  *(trx++) = '^';
  for (char * tmp = next; *tmp; ++tmp) {
	switch (*tmp) {
	case '*':
	  *(trx++) = '.';
	  *(trx++) = '*';
	  break;
	case '?':
	  *(trx++) = '.';
	  break;
	case '.':
	  *(trx++) = '\\';
	  *(trx++) = '.';
	case '/':
	  break;
	default:
	  *(trx++) = *tmp;
	}
  } *(trx++) = '$'; *(trx++) = 0;
  // Compile regex.
  regex_t * p_rgx = new regex_t();
  if (regcomp(p_rgx, rgx, REG_EXTENDED|REG_NOSUB)) {
	// Exit with error if regex failed to compile.
	perror("regex (compile)");
	exit(1);
  }

  char * _dir = (prefix) ? strdup(prefix) : strdup(".");

  DIR * dir = opendir(_dir); free(_dir);
  if (!dir) {
	free(dir); free(rgx); free(next); free(nextPrefix); delete p_rgx;
	return; // Return if opendir returned null.
  }

  dirent * _entry;
  for (; (_entry = readdir(dir));) {
	// Check for a match!
	if (!regexec(p_rgx, _entry->d_name, 0, 0, 0)) {
	  if (!(prefix && *prefix)) sprintf(nextPrefix, "%s", _entry->d_name);
	  else sprintf(nextPrefix, "%s/%s", prefix, _entry->d_name);

	  if (_entry->d_name[0] == '.' && *rgx == '.') wildcard_expand(nextPrefix, suffix);
	  else if (_entry->d_name[0] == '.') continue;
	  else wildcard_expand(nextPrefix, suffix);
	}
  }

  // Be kind to malloc. Malloc is bae.
  free(next); free(nextPrefix); free(rgx); free(dir);
  // This one was allocated with new.
  delete p_rgx;
}
