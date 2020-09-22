let digConstructor = (~env, ~getModule, path) => {
  switch (Query.resolveFromCompilerPath(~env, ~getModule, path)) {
  | `Not_found => None
  | `Stamp(stamp) =>
    let%opt t = Hashtbl.find_opt(env.file.stamps.types, stamp);
    Some((env, t));
  | `Exported(env, name) =>
    let%opt stamp = Hashtbl.find_opt(env.exported.types, name);
    let%opt t = Hashtbl.find_opt(env.file.stamps.types, stamp);
    Some((env, t));
  | _ => None
  }
};

let showModuleTopLevel = (~name, ~markdown, topLevel: list(SharedTypes.declared(SharedTypes.Module.item))) => {
  let contents =
    topLevel
    |. Belt.List.map(item =>
         switch (item.contents) {
         /** TODO pretty print module contents */
         | Module(_) => "  module " ++ item.name.txt ++ ";"
         | Type({decl}) =>
           "  "
           ++ (decl |> Shared.declToString(item.name.txt))
         | Value(typ) =>
           "  let "
           ++ item.name.txt
           ++ ": "
           ++ (typ |> Shared.typeToString) /* TODO indent */
           ++ ";"
         | ModuleType(_) => "  module type " ++ item.name.txt ++ ";"
         }
       )
    |> String.concat("\n");
  let full = "module " ++ name ++ " = {" ++ "\n" ++ contents ++ "\n}";
  Some(markdown ? "```\n" ++ full ++ "\n```" : full);
};

let showModule = (~markdown, ~file: SharedTypes.file, ~name, declared: option(SharedTypes.declared(SharedTypes.Module.kind))) => {
  switch declared {
    | None => showModuleTopLevel(~name, ~markdown, file.contents.topLevel)
    | Some({contents: Structure({topLevel})}) => {
      showModuleTopLevel(~name, ~markdown, topLevel)
    }
    | Some({contents: Ident(_)}) => Some("Unable to resolve module reference")
  }
};

open Infix;
let newHover = (~rootUri, ~file: SharedTypes.file, ~getModule, ~markdown, ~showPath, loc) => {
  switch (loc) {
    | SharedTypes.Loc.Explanation(text) => Some(text)
    /* TODO store a "defined" for Open (the module) */
    | Open => Some("an open")
    | TypeDefinition(_name, _tdecl, _stamp) => None
    | Module(LocalReference(stamp, _tip)) => {
      let%opt md = Hashtbl.find_opt(file.stamps.modules, stamp);
      let%opt (file, declared) = References.resolveModuleReference(~file, ~getModule, md);
      let name = switch declared {
        | Some(d) => d.name.txt
        | None => file.moduleName
      };
      showModule(~name, ~markdown, ~file, declared)
    }
    | Module(GlobalReference(moduleName, path, tip)) => {
      let%opt file = getModule(moduleName);
      let env = {Query.file, exported: file.contents.exported};
      let%opt (env, name) = Query.resolvePath(~env, ~path, ~getModule);
      let%opt stamp = Query.exportedForTip(~env, name, tip);
      let%opt md = Hashtbl.find_opt(file.stamps.modules, stamp);
      let%opt (file, declared) = References.resolveModuleReference(~file, ~getModule, md);
      let name = switch declared {
        | Some(d) => d.name.txt
        | None => file.moduleName
      };
      showModule(~name, ~markdown, ~file, declared)
    }
    | Module(_) => None
    | TopLevelModule(name) => {
      let%opt file = getModule(name);
      showModule(~name=file.moduleName, ~markdown, ~file, None)
    }
    | Typed(_, Definition(_, Attribute(_) | Constructor(_))) => None
    | Constant(t) => {
      Some(switch t {
      | Const_int(_) => "int"
      | Const_char(_) => "char"
      | Const_string(_) => "string"
      | Const_float(_) => "float"
      | Const_int32(_) => "int32"
      | Const_int64(_) => "int64"
      | Const_nativeint(_) => "int"
      })
    }
    | Typed(t, _) => {
      let typeString = t |> Shared.typeToString;
      let extraTypeInfo = {
        let env = {Query.file, exported: file.contents.exported};
        let%opt path = t |> Shared.digConstructor;
        let%opt (_env, {name: {txt}, contents: {decl}}) = digConstructor(~env, ~getModule, path);
        Some(decl |> Shared.declToString(txt))
        /* TODO type declaration */
        /* None */
        /* Some(typ.toString()) */
      };

      let codeBlock = text => markdown ? "```\n" ++ text ++ "\n```" : text;
      let typeString = codeBlock(typeString);
      let typeString = typeString ++ (switch (extraTypeInfo) {
        | None => ""
        | Some(extra) => "\n\n" ++ codeBlock(extra)
      });

      Some({
        let%opt ({docstring}, {uri}, res) = References.definedForLoc(
          ~file,
          ~getModule,
          loc,
        );

        let uri = Utils.startsWith(uri, rootUri)
          ? "<root>" ++ Utils.sliceToEnd(uri, String.length(rootUri))
          : uri;

        let parts = switch (res) {
          | `Declared => {
            [Some(typeString), docstring]
          }
          | `Constructor({name: {txt}, args}) => {
            [Some(typeString),
            Some(codeBlock(txt ++ "(" ++ (args |. Belt.List.map(((t, _)) => {
              let typeString =
                t |> Shared.typeToString;
              typeString

            }) |> String.concat(", ")) ++ ")")),
            docstring]
          }
          | `Attribute(_) => {
            [Some(typeString), docstring]
          }
        };

        let parts = showPath ? parts @ [Some(uri)] : parts;

        Some(String.concat("\n\n", parts |. Belt.List.keepMap(x => x)))
      } |? typeString)

    }
  };
};