open Lexing

type location = Warnings.loc = { loc_start: position; loc_end: position; loc_ghost: bool }

type 'a loc = 'a Location.loc = {
  txt : 'a;
  loc : location;
}

type longident = Longident.t =
    Lident of string
  | Ldot of longident * string
  | Lapply of longident * longident

type abstract_ident = Ident.t

type ident_408 =
  | Local of { name: string; stamp: int }
  | Scoped of { name: string; stamp: int; scope: int }
  | Global of string
  | Predef of { name: string; stamp: int }

let ident_binding_time_408 (ident: Ident.t) =
  let current_ident = (Obj.magic ident : ident_408) in
  match current_ident with
  | Predef { stamp }
  | Scoped { stamp }
  | Local { stamp } -> stamp
  | Global _ -> 0

let none = Location.none
let mknoloc = Location.mknoloc

type path = Path.t =
    Pident of Ident.t
  | Pdot of path * string
  | Papply of path * path

module Ident406 = struct
  type t = { stamp: int; name: string; mutable flags: int }
  let toIdent {name; stamp} = (Obj.magic(Local({name; stamp})) : Ident.t)

  let same (i1:t) (i2:t) = i1 = i2
end

module Path406 = struct
  type t =
      Pident of Ident406.t
    | Pdot of t * string * int
    | Papply of t * t

  let rec toPath (p:t) = match p with
    | Pident(i) -> Path.Pident(i |> Ident406.toIdent)
    | Pdot(p, s, _) -> Path.Pdot(p |> toPath, s)
    | Papply(p1, p2) -> Path.Papply(p1 |> toPath, p2 |> toPath)

  let rec name = function
    Pident id -> id.name
  | Pdot(p, s, _pos) ->
      name  p ^ "." ^ s
  | Papply(p1, p2) -> name p1 ^ "(" ^ name p2 ^ ")"

  let rec same p1 p2 =
    match (p1, p2) with
      (Pident id1, Pident id2) -> Ident406.same id1 id2
    | (Pdot(p1, s1, _), Pdot(p2, s2, _)) -> s1 = s2 && same p1 p2
    | (Papply(fun1, arg1), Papply(fun2, arg2)) ->
         same fun1 fun2 && same arg1 arg2
    | (_, _) -> false
  
end

let rec samePath p1 p2 =
  match (p1, p2) with
    (Pident id1, Pident id2) -> Ident.same id1 id2
  | (Pdot(p1, s1), Pdot(p2, s2)) -> s1 = s2 && samePath p1 p2
  | (Papply(fun1, arg1), Papply(fun2, arg2)) ->
       samePath fun1 fun2 && samePath arg1 arg2
  | (_, _) -> false

type constant = Asttypes.constant =
    Const_int of int
  | Const_char of char
  | Const_string of string * string option
  | Const_float of string
  | Const_int32 of int32
  | Const_int64 of int64
  | Const_nativeint of nativeint

type payload = Parsetree.payload =
  | PStr of Parsetree.structure
  | PSig of Parsetree.signature (* : SIG *)
  | PTyp of Parsetree.core_type  (* : T *)
  | PPat of Parsetree.pattern * Parsetree.expression option  (* ? P  or  ? P when E *)

module Parsetree = Parsetree
module Lexing = Lexing
module Parser = Parser
module Lexer = Lexer
