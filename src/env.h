#ifndef ENV_H
#define ENV_H

#ifndef VALUE_H
typedef struct value_s *value;
#endif

typedef struct env_s *env;

env env_new (env e);
void env_set (env e, value sym, value val);
value env_lookup (env e, const char *name);
value env_exists (env e, const char *name);
void env_dump (env e);
env env_extend (env parent, value params, value args);


#endif
