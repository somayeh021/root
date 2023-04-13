/*
 * Project: RooFit
 * Authors:
 *   Garima Singh, CERN 2023
 *   Jonas Rembser, CERN 2023
 *
 * Copyright (c) 2023, CERN
 *
 * Redistribution and use in source and binary forms,
 * with or without modification, are permitted according to the terms
 * listed in LICENSE (http://roofit.sourceforge.net/license.txt)
 */

#ifndef RooFit_Detail_CodeSquashContext_h
#define RooFit_Detail_CodeSquashContext_h

#include <RooAbsArg.h>
#include <RooNumber.h>
#include <RooAbsData.h>

#include <sstream>
#include <string>
#include <unordered_map>

template <class T>
class RooTemplateProxy;

namespace RooFit {

namespace Detail {

/// @brief A class to maintain the context for squashing of RooFit models into code.
class CodeSquashContext {
public:
   CodeSquashContext(const RooAbsData *data = nullptr) : _numEntries(data ? data->numEntries() : 0) {}

   inline void addResult(RooAbsArg const *key, std::string const &value) { addResult(key->namePtr(), value); }

   void addResult(const char *key, std::string const &value);

   std::string const &getResult(RooAbsArg const &arg);

   template <class T>
   std::string const &getResult(RooTemplateProxy<T> const &key)
   {
      return getResult(key.arg());
   }

   void addToGlobalScope(std::string const &str);
   std::string assembleCode(std::string const &returnExpr);
   void addVecObs(const char *key, int idx);

   /// @brief Adds the input string to the squashed code body. If a class implements a translate function that wants to
   /// emit something to the squashed code body, it must call this function with the code it wants to emit.
   /// @param in String to add to the squashed code.
   inline void addToCodeBody(std::string const &in) { _code += in; }

   /// @brief Build the code to call the function with name `funcname`, passing some arguments.
   /// The arguments can either be doubles or some RooFit arguments whose
   /// results will be looked up in the context.
   template <typename... Args_t>
   std::string buildCall(std::string const &funcname, Args_t const &...args)
   {
      std::stringstream ss;
      ss << funcname << "(" << buildArgs(args...) << ")" << std::endl;
      return ss.str();
   }

   /// @brief A class to manage loop scopes using the RAII technique. To wrap your code around a loop,
   /// simply place it between a brace inclosed scope with a call to beginLoop at the top. For e.g.
   /// {
   ///   auto scope = ctx.beginLoop({<-set of vector observables to loop over->});
   ///   // your loop body code goes here.
   /// }
   class LoopScope {
   public:
      LoopScope(CodeSquashContext &ctx, std::vector<TNamed const *> &&vars) : _ctx{ctx}, _vars{vars} {}
      ~LoopScope() { _ctx.endLoop(*this); }

      std::vector<TNamed const *> const &vars() const { return _vars; }

   private:
      CodeSquashContext &_ctx;
      const std::vector<TNamed const *> _vars;
   };

   std::unique_ptr<LoopScope> beginLoop(RooArgSet const &loopVars);

private:
   void endLoop(LoopScope const &scope);

   inline void addResult(TNamed const *key, std::string const &value) { _nodeNames[key] = value; }

   std::string buildArg(double x) { return RooNumber::toString(x); }

   template <class T>
   std::string buildArg(T const &arg)
   {
      return getResult(arg);
   }

   std::string buildArgs() { return ""; }

   template <class Arg_t>
   std::string buildArgs(Arg_t const &arg)
   {
      return buildArg(arg);
   }

   template <typename Arg_t, typename... Args_t>
   std::string buildArgs(Arg_t const &arg, Args_t const &...args)
   {
      return buildArg(arg) + ", " + buildArgs(args...);
   }

   /// @brief A crude way of keeping track of loop ranges for nodes like NLLs.
   const std::size_t _numEntries = 0;
   /// @brief Map of node names to their result strings.
   std::unordered_map<const TNamed *, std::string> _nodeNames;
   /// @brief Block of code that is placed before the rest of the function body.
   std::string _globalScope;
   /// @brief A map to keep track of the observable indices if they are non scalar.
   std::unordered_map<const TNamed *, int> _vecObsIndices;
   /// @brief Stores the squashed code body.
   std::string _code;
   /// @brief The current number of for loops the started.
   int _loopLevel = 0;
};

} // namespace Detail

} // namespace RooFit

#endif
