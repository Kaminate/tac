# Style Guide

---

## prefer not to inline fns

prioritize keeping the api easier to understand rather than
keeping total h + cpp code count low.


For constexpr functions, which become inline, their body must be visible in every translation unit
that uses it, they should be declared outside the struct in the .h file.
```cpp
// .h file
struct StringView
{
  constexpr StringView( const char* , int );
};

// still the .h file
constexpr StringView::StringView( const char* str, int len ) : mStr{ str }, mLen{ len } {}
```

## Single line ifs

Okay, except macros must be wrapped in brackets

Not okay:
```cpp
if( foo ) bar();

if( foo ) { bar() };

if( foo )            // not okay
  TAC_BAR; 
```

Okay:
```
if( foo )
  bar(); 

if( foo )
{
  TAC_BAR; 
}
```

## scope guards

use `TAC_SCOPE_GUARD`

## Use of auto

**Do** use `auto` to avoid repeating types
`auto foo { TAC_NEW Foo }`

**Do** use `auto` to avoid a type nobody cares about
`auto it { myMap.begin() }; // Map< Foo, Bar >::Iterator `

**Do not** use `auto` when it hides a type we potentially care about
`auto whatIsThisType { SomeFnCall() }`

use of auto for template fuckery is ok

---

## Enums to strings

Write a `StringView EnumNameToString( EnumName )` that uses a simple switch statement, with a `default: TAC_ASSERT_INVALID_CASE`. This is all temporary anyways once reflection is part of the c++ std library (hopefully).

---

## Initializers

Prefer to initialize variables with braces, leaving `=` for assignment.

```cpp
Foo f{ bar() }; // good
Foo f = bar();  // bad
```

However, prefer calling constructors with `Foo(bar)` over `Foo{bar}`.
^ this rule is currently in contention while i experiment with
| `return { bar, qux };` over `return Foo( bar, qux );`.
| I think the latter is better, but I changed it to the former cuz i was
| just complying with whatever visual assist x told me to do.


---


