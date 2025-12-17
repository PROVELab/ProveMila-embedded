// build.rs

fn main() {
    // 1. Compile C code (Add programConstants.c if you have one, or just the header path)
    cc::Build::new()
        .file("c_src/pecan_common.c")
        // If programConstants has a .c file, add it here too:
        // .file("c_src/programConstants.c") 
        .include("c_src")
        .flag("-std=c99")
        .flag("-mcpu=cortex-m4")
        .flag("-mthumb")
        .flag("-mfloat-abi=hard")
        .flag("-ffunction-sections")
        .flag("-fdata-sections")
        .compile("pecan_common"); // The library name

    // Re-run if either header changes
    println!("cargo:rerun-if-changed=c_src/pecan.h");
    println!("cargo:rerun-if-changed=c_src/programConstants.h");
    println!("cargo:rerun-if-changed=c_src/wrapper.h");

    // 2. Generate Bindings
    let bindings = bindgen::Builder::default()
        // Point to the wrapper that includes BOTH files
        .header("c_src/wrapper.h") 
        
        .use_core()
        .ctypes_prefix("core::ffi")
        .clang_arg("--target=thumbv7em-none-eabihf")
        .clang_arg("-mcpu=cortex-m4")
        .clang_arg("-mfloat-abi=hard")
        .clang_arg("-mfpu=fpv4-sp-d16")
        
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .derive_default(true)
        .generate()
        .expect("Unable to generate bindings");

    let out_path = std::path::PathBuf::from(std::env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}