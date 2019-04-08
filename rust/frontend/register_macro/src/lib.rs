#![recursion_limit = "128"]
#![allow(unused)]

extern crate proc_macro;

use proc_macro::TokenStream;
use quote::quote;
use syn::{parse_macro_input, ItemFn};

#[proc_macro_attribute]
pub fn register_global_func(_attr: TokenStream, item: TokenStream) -> TokenStream {
    let func = parse_macro_input!(item as ItemFn);
    let ident = &func.ident;
    let block = &func.block;
    let decl = &func.decl;
    let fn_token = &decl.fn_token;
    let inputs = &decl.inputs;
    let output = &decl.output;

    let reg_fn = quote! {
        #fn_token #ident(#inputs) #output #block
    };

    let registered = quote! {
        use ::tvm_frontend::function::register;
        register(#reg_fn, stringify!("{}", #ident).to_owned(), false).unwrap();
    };

    registered.into()
}
