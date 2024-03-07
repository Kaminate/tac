# Style Guide

---

## prefer not to inline fns

prioritize keeping the api easier to understand rather than
keeping total h + cpp code count low

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

Auto should be used if it avoids a type nobody cares about
`auto it = ... // Map< Foo, Bar >::Iterator `

Auto can be used if it avoids repeating types
`auto foo = new Foo` 

Auto should not be used when it hides useful type information
`auto whatIsThisType = SomeFnCall()`

use of auto for template fuckery is ok

---



