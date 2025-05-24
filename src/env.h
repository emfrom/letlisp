#ifndef ENV_H
#define ENV_H

#ifndef VALUE_H
typedef struct value_s *value;
#endif

typedef struct env_s *env;

env env_new(env e);
void env_set(env e, value sym, value val);


#endif
