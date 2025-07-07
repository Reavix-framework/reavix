#include "router.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

static Router router = {0};

bool router_init(size_t initial_capacity) {
    if (router.root != NULL) {
        return false;
    }

    router.root = calloc(1, sizeof(TrieNode));
    if (!router.root) {
        return false;
    }

    router.routes = malloc(initial_capacity * sizeof(RouteEntry));
    if (!router.routes) {
        free(router.root);
        return false;
    }

    router.route_capacity = initial_capacity;
    router.route_count = 0;
    pthread_mutex_init(&router.mutex, NULL);
    return true;
}

void router_free() {
    pthread_mutex_lock(&router.mutex);
    
    // Free trie nodes recursively
    free_trie_node(router.root);
    
    // Free route entries
    for (size_t i = 0; i < router.route_count; i++) {
        free(router.routes[i].method);
        free(router.routes[i].path);
    }
    free(router.routes);
    
    pthread_mutex_unlock(&router.mutex);
    pthread_mutex_destroy(&router.mutex);
    memset(&router, 0, sizeof(Router));
}

static void free_trie_node(TrieNode* node) {
    if (!node) return;
    
    free(node->segment);
    
    for (size_t i = 0; i < node->child_count; i++) {
        free_trie_node(&node->children[i]);
    }
    
    free(node->children);
    free_trie_node(node->param_child);
    free(node);
}

static TrieNode* create_trie_node(const char* segment) {
    TrieNode* node = calloc(1, sizeof(TrieNode));
    if (!node) return NULL;

    node->segment = strdup(segment);
    if (!node->segment) {
        free(node);
        return NULL;
    }
    return node;
}

bool router_add(const char* method, const char* path, RouteHandler handler) {
    if (!method || !path || !handler) return false;

    pthread_mutex_lock(&router.mutex);

    // Check if route already exists
    for (size_t i = 0; i < router.route_count; i++) {
        if (strcmp(router.routes[i].method, method) == 0 &&
            strcmp(router.routes[i].path, path) == 0) {
            pthread_mutex_unlock(&router.mutex);
            return false;
        }
    }

    // Add to route table
    if (router.route_count >= router.route_capacity) {
        size_t new_capacity = router.route_capacity * 2;
        RouteEntry* new_routes = realloc(router.routes, new_capacity * sizeof(RouteEntry));
        if (!new_routes) {
            pthread_mutex_unlock(&router.mutex);
            return false;
        }
        router.routes = new_routes;
        router.route_capacity = new_capacity;
    }

    router.routes[router.route_count].method = strdup(method);
    router.routes[router.route_count].path = strdup(path);
    router.routes[router.route_count].handler = handler;
    router.route_count++;

    // Add to trie
    char* path_copy = strdup(path);
    if (!path_copy) {
        pthread_mutex_unlock(&router.mutex);
        return false;
    }

    char* saveptr;
    char* segment = strtok_r(path_copy, "/", &saveptr);
    TrieNode* current = router.root;

    while (segment) {
        bool is_param = (segment[0] == ':');
        TrieNode** target = is_param ? &current->param_child : NULL;
        bool found = false;

        if (!is_param) {
            for (size_t i = 0; i < current->child_count; i++) {
                if (strcmp(current->children[i].segment, segment) == 0) {
                    current = &current->children[i];
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            TrieNode* new_node = create_trie_node(segment);
            if (!new_node) {
                free(path_copy);
                pthread_mutex_unlock(&router.mutex);
                return false;
            }

            if (is_param) {
                current->param_child = new_node;
                current = new_node;
            } else {
                TrieNode* new_children = realloc(current->children, 
                    (current->child_count + 1) * sizeof(TrieNode));
                if (!new_children) {
                    free(new_node->segment);
                    free(new_node);
                    free(path_copy);
                    pthread_mutex_unlock(&router.mutex);
                    return false;
                }
                current->children = new_children;
                current->children[current->child_count] = *new_node;
                free(new_node);
                current = &current->children[current->child_count];
                current->child_count++;
            }
        }

        segment = strtok_r(NULL, "/", &saveptr);
    }

    current->handler = handler;
    free(path_copy);
    pthread_mutex_unlock(&router.mutex);
    return true;
}

bool router_match(const char* method, const char* path, 
                 PathParam* params, size_t* param_count,
                 RouteHandler* handler) {
    if (!path || !handler) return false;

    pthread_mutex_lock(&router.mutex);

    char* path_copy = strdup(path);
    if (!path_copy) {
        pthread_mutex_unlock(&router.mutex);
        return false;
    }

    char* saveptr;
    char* segment = strtok_r(path_copy, "/", &saveptr);
    TrieNode* current = router.root;
    *param_count = 0;

    while (segment) {
        bool found = false;

        // Check static children first
        for (size_t i = 0; i < current->child_count; i++) {
            if (strcmp(current->children[i].segment, segment) == 0) {
                current = &current->children[i];
                found = true;
                break;
            }
        }

        // Check parameter child
        if (!found && current->param_child) {
            if (params && *param_count < MAX_PARAMS) {
                params[*param_count].name = current->param_child->segment + 1;
                params[*param_count].value = strdup(segment);
                (*param_count)++;
            }
            current = current->param_child;
            found = true;
        }

        if (!found) {
            free(path_copy);
            pthread_mutex_unlock(&router.mutex);
            return false;
        }

        segment = strtok_r(NULL, "/", &saveptr);
    }

    *handler = current->handler;
    free(path_copy);
    pthread_mutex_unlock(&router.mutex);
    
    // Verify method matches
    if (method) {
        for (size_t i = 0; i < router.route_count; i++) {
            if (router.routes[i].handler == *handler && 
                strcmp(router.routes[i].method, method) == 0) {
                return true;
            }
        }
        return false;
    }
    
    return true;
}