/*
 * Copyright (C) 2020 Brandon Huddle
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <parsing/Parser.hpp>
#include <passes/BasicTypeResolver.hpp>
#include <passes/DeclInstantiator.hpp>
#include <passes/NamespacePrototyper.hpp>
#include <passes/BasicDeclValidator.hpp>
#include <passes/CodeProcessor.hpp>
#include <passes/NameMangler.hpp>
#include <namemangling/ItaniumMangler.hpp>
#include <codegen/CodeGen.hpp>
#include <passes/CodeTransformer.hpp>
#include <objgen/ObjGen.hpp>
#include <linker/Linker.hpp>
#include "Target.hpp"

using namespace gulc;

// TODO: We need an alternative to `isize` and `usize`. While they are good for pointers they are NOT good for IO.
//       With IO we might be working with files larger than can be indexed with `isize` and `usize`. Need to account for
//       this with maybe an `fsize`, `osize`, `ioffset`, `uoffset`, or something else. Or maybe just make sure to use
//       `u64` in these scenarios.

// TODO: Next features to work on:
//       * [POSTPONED] Finish adding support for extensions...
//       * Improve error checking for Decls post-instantiation
//       * Improve `const` solving (and simplify expressions as much as possible in trivial situations like `i + 2 + 2` == `i + 4`)
//       * `peekStartPosition` needs to handle removing any whitespace. Currently doing `peekStartPosition` for statements within a function will return `0, 0` for properly formatted code.

// TODO: Sidenotes:
//       * `ghoul` should handle creating new projects for us (i.e. `ghoul new Example`, `ghoul new --template=[URI] Example`)
//       * `ghoul` should handle the build files (e.g. replacement for `cmake`, should be able to also handle calling
//         the C compiler and other language compilers)
//       * Basically `ghoul` should be a compiler, a build system, and a project creation tool

// TODO: For templates I think we should stop modifying them after `DeclInstantiator` and instead create a fake
//       instantiation with special parameters to handle validating the template. This would help with:
//        1. Further processing of a template will make creating new instantiations difficult to impossible
//        2. There are some aspects of templated types that are impossible process further without instantiating
//        3. For the validation instantiation we can create a `ConceptType` that is used to validate everything with a
//           template works.

// TODO: I think we should use the terms `abandon` and `abandonment` instead of `panic` even though Rust currently uses
//       `panic`.
//       Pros:
//        * Most descriptive to what is happening. `panic` doesn't really tell you what is actually happening and could
//          give the wrong impression that the program could keep going. Saying the process `abandoned` sounds more
//          descriptive and doesn't leave any impression that it could keep going or be fixed.
//        * More user friendly. When a process "abandons" for a user-facing program the most likely thing to happen
//          will be for the program to tell the user the program abandoned and how to report that to the developer.
//          In my opinion `abandon` will give the user a better idea on how severe this is than `panic`. Telling the
//          user the program `panicked` seems more confusing.
//       Cons:
//        * Rust already has set the idea up as `panic` (even though earlier implementations called in `abandon` as
//          well, such as the Midori OS's programming language)
//        * It seems like Go also uses `panic`

// TODO: We need to clean up how we handle local variables. Anywhere where we access them we are manually regenerating
//       the list for the context. I think we could do better than that by storing a list for each context
// TODO: Templates need extended to support specialization. The `where` should NOT specialize.
//           struct partial_ref<T> { var m: ref T }
//           struct partial_ref<T: i32> { var m: T }
//       The idea is mainly just to give a way to specialize a template for certain types that might be better off
//       treated differently.


// TODO: We need to finish the following:
//        7. We need to have a test `Template*InstDecl` that creates an `ImaginaryType` constructed based on both the
//           specialized type (for `baseStruct` and the `has *` for the rest of the members.)
//        8. We need to validate template specialization works in `DeclInstantiator` when the `<T: vec3>` is used
//           before `vec3` and `vec3i` are declared in the file with:
//               struct Example<T: vec3> {}
//               let test: Example<vec3i>
//               struct vec3 {}
//               struct vec3i: vec3 {}
// TODO: For specialization if there are `T: GrandParent` and `T: Parent` and you pass `Child` it should use
//       `parent depth` in the inheritance list to choose. The one that is closer to `Child` in the list is the one we
//       choose. So in this case we choose `T: Parent` instead of `T: GrandParent` since it goes
//       `Child: Parent: Grandparent`
// TODO: Support `trait`
// TODO: Support `<=>`
// TODO: Finish extensions
// TODO: Finish static-array/flat-array support. (e.g. `[i32; 12]`)
// TODO: Finish literal suffixes
//       To do this we will need something like such:
//           public typesuffix str<const length: usize>(_ input: [u8; N]) -> string { ... }
//       We will need to make type suffixes templates to allow for n-length character arrays.
//       The above would then allow you to do:
//           let strLiteral = "hello!"str
//       With the `str` suffix `strLiteral` will be of type `string`, without `str` then `strLiteral` will be of type
//       `[u8; N]`
// TODO: Finish `DimensionType` so we can declare `[]i32`, `[,,]*bool`, etc.
// TODO: Support tuples (put on hold due to how much needs to be changed for tuples to be supported. They might be
//       better off being added to the bootstrapped compiler instead of now...)
// TODO: Support top-level-statements. This would allow the below:
//           import std.io
//           Console.write("Hello, world!")
//       The way this would work is by using a `TopLevelStmtDecl` which would all be combined into an auto-generated
//       "func main() -> void"
//       It would have to follow these rules:
//           1. Top-level statements can only appear in a single file of a project (not multiple, that would be nearly
//              impossible to solve properly and would be too ambiguous on when any code gets called)
//           2. You CANNOT mix top-level statements and a custom "func main() -> void".
//           3. `var` rules still apply. You MUST have `static var` or `const var`, `var` is NOT allowed at top-level
//           4. `const var` CANNOT reference top-level `let`
//       We will need to answer the following:
//           1. Can `static var` reference top-level `let`? It is translatable...
//           2. Do we auto initialization of `static var` when using top-level statements? Without using TLS you would
//              assign the `static var` manually but there isn't a `main` to do it manually with top-level statements
//           3. Should we have a `static var std.os.args: ref []string` to allow CLI argument access from TL-Stmt?
// TODO: I think we should go hardcore into English keywords and pattern matching. I've been thinking about this for a
//       while but have been worried others won't like it. But I've realized this language is for me. If no one else
//       likes it who cares.
//       We should remove any non-English boolean logic operators (except the maths ones)
//       Examples of what I mean:
//           if t is not i32 {}
//           if t is null {}
//           if t is not null {}
//           switch someValue {
//               case is i32: //
//               case > 0 and < 12: // someValue > 0 and someValue < 12
//               case let (x, y): // Only if `someValue` is a tuple...
//               case (0, _): // If `someValue.0 == 0`
//               case (12, 44): // If `someValue.0 == 12 and someValue.1 == 44`
//           }
//           // This might be difficult to parse properly... But if we can do this it would be awesome
//           if t is > 0 and (< 12 or 44) // t > 0 && (t < 12 || t == 44)
//           if t is <= 44 and let x: i32 // if t <= 44 then let x: i32 = t.value { ... } is this needed?
//           // For the above we would have to make it only work with the boolean operators and no-custom operators.
//           // So `[ >, >=, <, <=, <=>, not, ==, != ]`
//           // Continuing...
//           let x: ?i32 = null
//           if let xValue = x // Only works when `x` isn't null, when `x` isn't null `xValue` contains the value
//           let nullArray: []?i32 = [ null, 12, 44, null, 0 ]
//           @for let value in nullArray // Skips any null indexes, assigns `value` the `i32` type when the index isn't null
//           // Also we should support the parsing order where `and` is parsed before `or` to allow:
//           if i is >= 12 and <= 16 or >= 28 and <= 32
//           // The above should be equal to:
//           if i is (>= 12 and <= 16) or (>= 28 and <= 32)
// TODO: Since we'll already be changing the parser for more english words, it could be beneficial to go ahead and
//       redesign the parser to do the following:
//           * Combine ALL expressions into a `RawListExpr` that contains every symbol individually. That way we can
//             allow macros and custom operators
//           * Create a new compiler pass to handle converting expression lists into their final form
//             (i.e. `i is >= 12 and <= 16` to `i >= 12 and i <= 16)`)
// TODO: We need to start planning the support for `async`/`await`:
//        * Do we go the "lazy" route like Rust? Calling an async function only creates the `future` it doesn't
//          schedule it?
//        * Do we make this key to the actual compiler? For the `future` result that goes against what I want but might
//          be necessary
//        * Do we want to try to think of a better way to do this? I've heard a lot of doubts about async/await and I'm
//          not sure there could be anything better. But it would be interesting to try to think of a better way to
//          handle it.

// TODO: Improve error messages (e.g. add checks to validate everything is how it is supposed to be, output source code
//       of where the error went wrong, etc.)

// TODO: I think we should support `operator infix .()` or `operator deref` to allow smart references.
//       C++ is starting to move towards the idea of `operator .()` and I really like the idea. I think this would be
//       nicer though:
//           struct box<T> {
//               private var ptr: *T
//  .
//               pure operator prefix deref() -> &T
//               requires ptr != null {
//                   return *T
//               }
//           }
//       The above would allow us to do:
//           let v: box<Window> = @new Window(title: "Hello, World!")
//  .
//           v.show()
//       While typing `v->show()` wouldn't be that big of a deal I really just don't want to require that. `box<T>`
//       is just a smart reference to a static reference, we shouldn't need to require `->`.
//       I think we should do `operator prefix deref()` and `operator prefix *()`
//       instead of `operator infix .()` and `operator infix ->()` to allow for us to us to call ALL operators on the
//       underlying reference type in a more natural/intuitive way. `operator infix .()` doesn't make me think that
//       `box<i32> + box<i32>` will call `operator infix +(_ rightValue: i32)`. BUT `operator prefix deref` does since
//       you will have to `deref` for `operator infix +(_ rightValue: i32)`.
//       To support this I think the general path for working with smart references is as follows:
//           if lValue { convertLValueToRValue(...) }
//           if smartRef { dereferenceSmartReferences(...) }
//           if ref { dereferenceReferences(...) }
//       Detection will obviously need to be a little more in depth but the above should work how it is needed. Unlike
//       normal references, we would double dereference. Smart references would be a layer above references.
//       I.e.:
//           let dRef: ref ref i32 = ...
//           let error: i32 = dRef // ERROR: `dRef` can only directly become `ref i32`, we won't double dereference
//           // BUT
//           let sRef: Ref<i32> = ...
//           let ok: i32 = sRef // OK: `Ref<i32>::operator prefix deref() -> ref i32` will automagically also dereference
//                              //     the result of the first `deref`. We only do that because returning `ref` is more natural.
//       NOTE: Overloading `operator prefix deref()` will make it so no other operators besides the following are allowed:
//           * `operator as<T>()`
//           * `operator is<T>()`
//       Any other operator would be pass to `T` instead. The reason for `as` and `is` being supported is to allow the
//       smart reference to implement basic support for runtime type reflection etc.
//       How would you support functions on a smart reference? Easy. You don't. A smart reference can't have functions.
//       If you DO add functions the only way to access them would be through full path signatures:
//           struct box<T> {
//               func getPointer() -> *T { ... }
//           }
//           let window: box<Window> = ...
//           let windowPtr: *Window = box<Window>::getPointer(window)

// TODO: We need to disable argument labels on these functors:
//        * Operators
//        * Type alias
//        * Type suffix

// TODO: We need to support initial value on properties like in C#:
//           struct Example {
//               prop Defaulted: i32 { get; set } = 12
//           }
//       And
//           namespace example {
//               prop Defaulted: i32 { get; set } = 12
//           }
//       One thing to note is that a `MemberProperty` can have a "runtime expression" for the initial value but the
//       normal `GlobalProperty` requires the initial value to be const-solvable same as a normal `const var` or
//       `static var` initial value.

// TODO: Cleanup name mangling for `prop` and `subscript`

// TODO: We should probably rename `FlatArrayType` to `StaticArrayType`. Technically our dynamic array that we will be
//       making will be a `FlatArray`, `StaticArray` makes much more sense imo..


int main() {
    Target target = Target::getHostTarget();

    std::vector<std::string> filePaths {
            "examples/TestFile.ghoul"
//            "examples/TemplateWhereContractTest.ghoul"
    };
    std::vector<ASTFile> parsedFiles;

    for (std::size_t i = 0; i < filePaths.size(); ++i) {
        Parser parser;
        parsedFiles.push_back(parser.parseFile(i, filePaths[i]));
    }

    // Generate namespace map
    NamespacePrototyper namespacePrototyper;
    std::vector<NamespaceDecl*> prototypes = namespacePrototyper.generatePrototypes(parsedFiles);

    // Validate imports, check for obvious redefinitions, set the `Decl::container` member, etc.
    // TODO: Should we rename this to `BasicValidator` and move the label validation to here?
    //       I would go ahead and do it but I'm not sure if the overhead is worth it, I think it would be better
    //       to wait until we're doing more processing of the `Stmt`s...
    BasicDeclValidator basicDeclValidator(filePaths, prototypes);
    basicDeclValidator.processFiles(parsedFiles);

    // Resolve all types as much as possible, leaving `TemplatedType`s for any templates
    BasicTypeResolver basicTypeResolver(filePaths, prototypes);
    basicTypeResolver.processFiles(parsedFiles);

    // Instantiate Decl instances as much as possible (set `StructDecl` data layouts, instantiate `TemplatedType`, etc.)
    DeclInstantiator declInstantiator(target, filePaths);
    declInstantiator.processFiles(parsedFiles);

    // TODO: We need to actually implement `DeclInstValidator`
    //        * Check to make sure all `Self` type references are removed and are valid
    //        *

    // Process main code before IR generation
    CodeProcessor codeProcessor(target, filePaths, prototypes);
    codeProcessor.processFiles(parsedFiles);

    // Mangle decl names for code generation
    auto manglerBackend = ItaniumMangler();
    NameMangler nameMangler(&manglerBackend);
    nameMangler.processFiles(parsedFiles);

    // TODO: I think we could parallelize this and `CodeGen` since they don't modify any `Decl`
    CodeTransformer codeTransformer(target, filePaths, prototypes);
    codeTransformer.processFiles(parsedFiles);

    std::vector<ObjFile> objFiles;
    objFiles.reserve(parsedFiles.size());

    ObjGen::init();
    ObjGen objGen = ObjGen();

    for (auto parsedFile : parsedFiles) {
        // Generate LLVM IR
        CodeGen codeGen(target, filePaths);
        gulc::Module module = codeGen.generate(&parsedFile);

        // Generate the object files
        objFiles.push_back(objGen.generate(module));
    }

    gulc::Linker::link(objFiles);


    return 0;
}
