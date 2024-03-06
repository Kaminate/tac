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


