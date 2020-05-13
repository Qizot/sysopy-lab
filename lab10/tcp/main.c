//
// Created by jakub on 12.05.2020.
//
#include <stdio.h>

struct A {
    int type;
};

struct B {
    int type;
    char* name;
};

void hey(struct A* a) {
    printf("%d\n", a->type);
    struct B* b = a;
    printf("%s\n", b->name);
}

int main() {
    struct B b;
    b.type = 13;
    b.name = "fajna robota";
    hey(&b);
}

