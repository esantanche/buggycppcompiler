# Dealing with a bug in a C++ compiler

When porting a complex C++ application to Windows, compilers available on Windows weren't able to compile the code correctly.

## The problem

This complex application was using bit fields.

They are variables stored in only just a few bits in a memory word.

Imagine you have a variable that represents the day of the week. 

This variable has values between 0 and 6 where 0 is Sunday and 6 is Saturday.

We need just 3 bits to store this variable, so why waste an entire memory word? Most of its 32 or 64 bits will be wasted.

That's where bit fields come handy.

The problem is that C++ compilers weren't good at dealing with bit fields.

They were buggy.

After all they are software too and software has bugs.

## The solution

I had to discover the bug in the compiler we were using. I had to discover that the compiler wasn't properly run the statement i++ on a bit field.

This is difficult because we tend to think that a compiler compiles our code perfectly.

I tested many C++ compilers to find one that compiled bit fields correctly.

C++ compilers compile code in different ways. This means that you have to fix your code to fit the special way each compiler works.

It's useful to find a compiler that doesn't require you to make too many changes to your code.

## Folders

The folder PROCOMP-COMPAR-COMPILERS contains many C++ programs used to compare compilers.

Some of the compiler under test were: Symantec C++ compiler, VisualAge, Watcom, Cpp Builder.

## Technologies

* C++
* Many C++ compilers


