#include <parsing/Parser.hpp>
#include <passes/BasicTypeResolver.hpp>
#include <passes/DeclInstantiator.hpp>
#include <passes/NamespacePrototyper.hpp>
#include <passes/BasicDeclValidator.hpp>
#include <passes/CodeProcessor.hpp>
#include <passes/NameMangler.hpp>
#include <namemangling/ItaniumMangler.hpp>
#include <codegen/CodeGen.hpp>
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
//       * Output working executables through IR generation... sounds so quick right? Well hopefully it will be since we already have that from last years compiler...
//       * `peekStartPosition` needs to handle removing any whitespace. Currently doing `peekStartPosition` for statements within a function will return `0, 0` for properly formatted code.

// TODO: Sidenotes:
//       * `ghoul` should handle creating new projects for us (i.e. `ghoul new Example`, `ghoul new --template=[URI] Example`)
//       * `ghoul` should handle the build files (e.g. replacement for `cmake`, should be able to also handle calling
//         the C compiler and other language compilers)
//       * Basically `ghoul` should be a compiler, a build system, and a project creation tool

// TODO: We need to disable implicit string literal concatenation for `[ ... ]` array literals:
//       [
//           "This "
//           "should error on the above line saying it expected `,`"
//       ]
//       This should be allowed everywhere else EXCEPT in an array literal. If you use parenthesis the error goes away
//       [
//           ("This "
//           "is allowed sense it won't allow accidental concatenation")
//       ]
//       The reason we need to do this is because with us no longer needing `;` at the end of every line someone might
//       THINK we allow implicit `,` in this scenario. This also wouldn't error out so they would THINK everything
//       worked until runtime when they realize the array is now:
//       [ "This should error on the above line saying it expected `,`" ]
//       If we can avoid runtime errors I would like to as much as possible. So within `[ ... ]` array literals
//       disallow implicit string concatenation UNLESS the strings are contained within `(...)`.

// TODO: Remember to add bit-fields with `var example: 1` and any other numbers for proper bitfielding.

// TODO: Should we consider changing attributes to an `@...` system instead of `[...]` like Swift?
//       Pros:
//           * Less confusing syntax for parameters
//             `func test([Escaping] _ callback: func())`
//             vs
//             `func test(_ callback: @escaping func())`
//             We couldn't do `func test(_ callback: [Escaping] func())` as it would be confused for array syntax
//           * Could be extended to allow fully custom syntax
//             i.e. `@xmlClass Example {}` for an XML serializable class (AWFUL practice, don't do it. But basic
//             example.)
//           * Could potential be used to replace the rust macros `foreach!` stuff to allow:
//             @for i in 0..<12 {}
//           * More common. Swift, Kotlin, Java, etc. all use `@` nearly exactly the same way. C# uses `[...]`, Rust
//             uses `#[]`, C++ uses `[[...]]`, etc. It would be more uniform and well known to use `@...`
//       Cons:
//           * Uglier. Coming from C# I like `[XmlRoot] class AmazonEnvelope { ... }` better but I could grow to like
//             `@xmlRoot class AmazonEnvelope { ... }`
//           * That's really it. I just don't like how it looks. But having macros use `@...` as well would be nice I
//             guess. I do like `@for <item> in <iterator/enumerable> { ... }`

// TODO: For templates I think we should stop modifying them after `DeclInstantiator` and instead create a fake
//       instantiation with special parameters to handle validating the template. This would help with:
//        1. Further processing of a template will make creating new instantiations difficult to impossible
//        2. There are some aspects of templated types that are impossible process further without instantiating
//        3. For the validation instantiation we can create a `ConceptType` that is used to validate everything with a
//           template works.

// TODO: We should change `try {} catch {}` to `do {} catch {}` similar to Swift. The idea to make function calls
//       require a `try` prefix was initially inspired by `Midori` (the same language that inspired Ghoul's contracts
//       and a few other language features) but seeing Swift using the same idea was a pleasent surprise. I also liked
//       `do {} catch {}` better as:
//           try {
//               try someFunc();
//               try someFunc2();
//           } catch {
//               print("error")
//           }
//       Looks too repetitive.
//           do {
//               try someFunc();
//               try someFunc2();
//           } catch {
//               print("error")
//           }
//       Looks better only as a less repetitive option.

// TODO: Need to parse `true`, `false`, and `bool`

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

    for (auto parsedFile : parsedFiles) {
        CodeGen codeGen(target, filePaths);
        codeGen.generate(&parsedFile);
    }

    return 0;
}
