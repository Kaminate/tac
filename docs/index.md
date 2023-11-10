# Docs

---

## `TAC_SCOPE_GUARD`

2023-11-10 Together C & C++ Discord

```cpp
#define TAC_SCOPE_GUARD( var ) auto TAC_TMP_VAR_NAME = var
```

Given `struct Foo`, consider the difference between

```cpp
{
  Foo();   // A
  Foo _(); // B
  // ...
}
```

In summary, `Foo()` creates a temporary entity that dissapears by the end of the expression.
I'm gonna go ahead and say `Foo_()` is an lvalue with block scope

---

me
  what is this called

dot
  a function style conversion

nats
  temporary objects

me
  The opposite of a temporary object would be an object? Like an lvalue?

nats
  Sure, although in C++, an object is a temporary entity that wraps up before the scope does. If an object is a "object of the long-storage form," – this means it is not temporary and won't be destroyed when it leaves its scope, then it can be considered an "Lvalue"

slybach
  Those are automatic storage duration object, temporary has a more specific meaning, as does "lvalue"

dot
  `Foo()` is a function style conversion that produces a prvalue of type Foo. it's the sole expression in an expression statement, making it a discarded value expression. thus, temporary materialization is triggered, which creates the temporary object, which exists until the end of the full expression since its lifetime is not extended. there's pretty much the full story. ^^ 

nats
  > Those are automatic...
  Yeah sure, but in the context of C++, whatever object that fits under the category of "automatic storage duration" is considered to be "temporary" as opposed to "permanent." That's what's referred to as a "temporary entity" in more general terms in C++, and the reason for the "const" qualifier in C++, which clearly shows an object that the compiler will know cannot disappear entirely during execution and can therefore be kept by the pointer without having caused the pointer or the object to disappear

slybach
  The typical meaning of temporary in c++ refers to objects/entities that will immediately disappear by the end of the full expression that creates the entity

  You're the first person I've met who uses temporary to describe automatic storage duration, and the expression temporary in the standard also doesn't include automatic storage duration object

  The closest thing to "permanent" storage is arguably static storage duration

  Const qualifier isn't directly related to storage duration, and isn't directly related to temporaries or "permanent" objects

  There's hardly ever a context where you can safely grab a pointer to an object without considering the lifetime of that object
  And objects with the "longest" lifetimes are static storage duration objects, and even those you cannot guarantee will not go out of scope at some point because static order destruction fiasco is a thing

Nats
  Yes, but even though the static lifetime is the longest of the three storage times, not "permanent." Although its presence of static storage is guaranteed until program termination, precise duration of static storage's is not guaranteed

SlyBach
  I think we might be talking past each other a bit

---

maybe `Foo()` should be `Foo{}` to resolve ambiguity between constructing a foo and a function declaration

---
