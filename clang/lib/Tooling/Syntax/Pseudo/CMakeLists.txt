set(LLVM_LINK_COMPONENTS Support)

add_clang_library(clangToolingSyntaxPseudo
  Grammar.cpp
  GrammarBNF.cpp
  
  LINK_LIBS
  clangBasic
  clangLex
  )
