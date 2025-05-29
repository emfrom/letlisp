#include <string.h>

#include "env.h"
#include "value.h"
#include "memory.h"
#include "repl.h"
#include "builtin.h"


/**
 * Environment
 *
 * All env modifications are done by prepending, first occurence of symbol in the alist is
 *  the symbols value in that context
 *
 * Todo: Do I need to remove duplicates? (later, later)
 */
struct env_s
{
    env parent;                 // Outer scope environment (NULL for global)
    value bindings;             // alist of (symbol . value) pairs for this scope
};

env env_new (env parent)
{
    env e = gcx_malloc (sizeof (struct env_s));
    e->parent = parent;
    e->bindings = value_new_nil ();
    return e;
}

void env_set (env e, value sym, value val)
{
    // New
    value pair = value_new_cons (sym, val);
    e->bindings = value_new_cons (pair, e->bindings);
}

env env_extend (env parent, value params, value args)
{
    env e = env_new (parent);
    while (params->type == TYPE_CONS && args->type == TYPE_CONS)
    {
        env_set (e, car (params), car (args));
        params = cdr (params);
        args = cdr (args);
    }
    return e;
}

value env_exists (env e, const char *name)
{
    for (; e != NULL; e = e->parent)
        for (value bind = e->bindings; bind->type == TYPE_CONS;
             bind = cdr (bind))
        {
            value pair = car (bind);

            if (strcmp (car (pair)->sym, name) == 0)
            {
                return pair;
            }
        }

    return value_new_nil ();
}

value env_lookup (env e, const char *name)
{
    value v = env_exists (e, name);

    if (bool_isnil (v, e))
        repl_error ("Unbound symbol: %s\n", name);

    return cdr (v);
}

void env_dump (env e)
{
    //Debug
    value v = e->bindings;

    while (!bool_isnil (v, e))
    {
        value p = car (v);

        printf ("%s(%s): %p\n", car (p)->sym,
                car (p)->type == TYPE_SPECIAL ? "spec" : "sym", cdr (p));

        v = cdr (v);
    }
}
