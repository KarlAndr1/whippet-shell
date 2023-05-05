## Syntax

Whippet commands are built out of expressions. An expression is written as an operator or function followed by arguments all enclosed in brackets.
	(fn arg1 arg2 arg3)

For example, the following command will evaluate to the sum of 20 and 30
	(+ 20 30)

The operator and arguments may themselves be expressions:
	(* (+ 5 5) 10)
This will evaluate as (5 + 5) * 10

For the top level expression, the brackets are optional:
	+ 20 30
is the same as
	(+ 20 30)
and the earlier expression may also be written as
	* (+ 5 5) 10

# Strings

All characters (except brackets and whitespace) when written together form strings, with the exception of numbers (strings may contain numbers, but cannot
begin with a digit)
	foobar
is an example of a string. 

Using quotation marks, special characters and whitespace can be included in a string.
	"foo bar"

# Variables

To get the value stored inside a variable, write a sigil (@, $ or ') followed by a string giving the name of the variable.
	@var1
evaluates to the value stored in the variable "var1".

Note that sigils are not needed for functions;
	(fnvar x y)
calls the function stored in the variable "fnvar", not the literal string "fnvar".
