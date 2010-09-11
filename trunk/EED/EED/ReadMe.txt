Implementing the illusion of class references
---------------------------------------------

The user expects to use classes in the debugger in a similar way as in 
writing a program in D, which is by using class references. Class references 
are like pointers, but involve an implicit indirection in all operations.

Debug information, on the other hand, is meant to be explicit about all 
aspects of types and declarations. This involves always using a pointer to a 
class to represent a class reference.

These facts are at odds. In order to resolve that conflict, I introduced a 
new type called TypeReference. This type is meant to act in general like a 
pointer but with reference features where needed. If the library user wants 
to show his user class references instead of pointers to classes, then he 
must follow the following steps.

1. When returning a class declaration from MagoEE::IValueBinder::FindObject, 
   instead return a declaration that yields a type of reference to class.
2. When instantiating a type of pointer to class, instead return a type of 
   reference to class.

MagoEE::ITypeEnv has a new method NewReference to support this. References 
return true from the methods IsPointer and IsReference.

The following were the overall changes that were needed.

1. A declaration should say what kind of aggregate it is: struct, class, or 
   union.
2. Reference types should skip adding "*" during ToString.
3. Class member enumeration should skip the reference type of the parent 
   expression.
4. Allow dot expressions on pointers.
   - This is shared with struct and union types.
   - This is already implemented.
5. Disallow pointer and index expressions on references.
