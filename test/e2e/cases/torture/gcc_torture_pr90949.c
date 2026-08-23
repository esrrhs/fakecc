// expect: 0
package main;

struct Node
{
  struct Node *child;
};

struct Node space[2];

struct Node * __attribute__((noinline)) my_malloc(int bytes)
{
  return &space[0];
}

void __attribute__((noinline)) walk(struct Node *module, int cleanup)
{
  if (module == 0)
  {
    return;
  }
  walk (module->child, cleanup);
}

int main(void)
{
  struct Node *node = my_malloc(sizeof(struct Node));
  node->child = 0;
  walk (node, 1);
  return 0;
}
