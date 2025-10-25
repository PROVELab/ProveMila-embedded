

import os
from parseFile import dataPoint_fields, CANFrame_fields, vitalsNode_fields, globalDefines, globalEnums, ACCESS

import ast
import operator
# Safe evaluator for integer-only expressions and bitwise operators.
def eval_int_expr(expr: str) -> int:
    """
    Accepts strings like:
      "0b11<<3", "4<<1", "0xF>>2", "-(0b101<<2)"
    and returns the evaluated integer. Disallows names, function calls, etc.
    """
    node = ast.parse(expr, mode="eval")

    def _eval(n):
        if isinstance(n, ast.Expression):
            return _eval(n.body)

        # Python already parses 0b..., 0o..., 0x..., and decimal ints as ints
        if isinstance(n, ast.Constant) and isinstance(n.value, int):
            return n.value

        # Support unary + and -
        if isinstance(n, ast.UnaryOp) and isinstance(n.op, (ast.UAdd, ast.USub)):
            val = _eval(n.operand)
            return +val if isinstance(n.op, ast.UAdd) else -val

        # Support parentheses via AST structure automatically

        # Support shifts
        if isinstance(n, ast.BinOp) and isinstance(n.op, (ast.LShift, ast.RShift)):
            left = _eval(n.left)
            right = _eval(n.right)
            if not isinstance(left, int) or not isinstance(right, int):
                raise ValueError("Shift operands must be integers.")
            return operator.lshift(left, right) if isinstance(n.op, ast.LShift) else operator.rshift(left, right)

        # Other bitwise ops: |, &, ^
        if isinstance(n, ast.BinOp) and isinstance(n.op, (ast.BitOr, ast.BitAnd, ast.BitXor)):
            left = _eval(n.left); right = _eval(n.right)
            if isinstance(n.op, ast.BitOr):  return left | right
            if isinstance(n.op, ast.BitAnd): return left & right
            return left ^ right

        raise ValueError("Only integer literals (3, 0b11, 0x3) with <<, >>, &, ^, | are allowed.")

    return _eval(node)

import os

 # === Precompute node IDs in Python, for Java Constants
def genNodeIDsJava(f, missingIDs, numberOfNodes, startingOffset):

    nodeIDs = []
    mi = 0
    current = startingOffset
    while len(nodeIDs) < numberOfNodes:
        if mi < len(missingIDs) and current == missingIDs[mi]:
            mi += 1
            current += 1
            continue
        nodeIDs.append(current)
        current += 1

    node_elems = ", ".join(str(x) for x in nodeIDs)
    f.write("\n\tpublic static final int[] nodeIDs = new int[]{ " + node_elems + " };\n")

#write enums to file f, in either C or Java syntax    
def writeEnums(f, lang_l: str):
    if not globalEnums:
        return

    if lang_l == "c":
        for g in globalEnums:
            f.write(
                f'\n// global enum {g.enum_name}\n'
                f'typedef enum {{\n'
            )
            for i, e in enumerate(g.entries):
                raw = e.value.strip()
                try:
                    val = eval_int_expr(raw) if isinstance(raw, str) else int(raw)
                except Exception:
                    if isinstance(raw, str) and raw.startswith("0b"):
                        val = int(raw, 2)
                    else:
                        raise ValueError(f"Non-numeric enum value for {e.name}: {raw!r}")
                comma = ',' if i < len(g.entries) - 1 else ''
                f.write(f'\t{e.name} = {val}{comma}\t/* {raw} */\n')
            f.write(
                f'}} {g.enum_name};\n'
            )

    elif lang_l == "java":
        if not globalEnums:
            return

        for g in globalEnums:
            # Class header
            f.write(
                f'\n\t// global enum {g.enum_name}\n'
                f'\tpublic static final class {g.enum_name} {{\n'
                f'\t\tprivate {g.enum_name}() {{}}\n'
            )
            # Constants
            for e in g.entries:
                raw = e.value.strip()
                try:
                    val = eval_int_expr(raw) if isinstance(raw, str) else int(raw)
                except Exception:
                    if isinstance(raw, str) and raw.startswith("0b"):
                        val = int(raw, 2)
                    else:
                        raise ValueError(f"Non-numeric enum value for {e.name}: {raw!r}")
                f.write(f'\t\tpublic static final int {e.name} = {int(val)};\t// {raw}\n')
            # Close class
            f.write('\t}\n')


# Writes constants files for either C or java depending on lang
def writeConstants(lang, constants_file_path, minId, numMissingIDs, nodeCount, frameCount, globalDefines, missingIDs):
    lang_l = lang.lower()

    # Choose output path and wrappers
    if lang_l == "java":
        out_path = os.path.join(os.path.dirname(constants_file_path), "Constants.java")
        pre_wrapper_open  = "public final class Constants {\n    private Constants() {}\n\n"
        pre_wrapper_close = "}\n"
        # The ONLY difference in the main body: how each constant is emitted
        const_decl = "public static final int {name} = {value};"
        indent = "    "  # indent inside class
    elif lang_l == "c":
        out_path = constants_file_path
        pre_wrapper_open  = "#ifndef progConsts\n#define progConsts\n\n"
        pre_wrapper_close = "\n#endif\n"
        const_decl = "#define {name} {value}"
        indent = ""       # no extra indent in C
    else:
        raise ValueError(f"Unsupported lang: {lang!r}")

    with open(out_path, "w") as f:
        # Write wrapper open
        f.write(pre_wrapper_open)

        # ===== Main body (shared) =====
        f.write(f"{indent}//generated Constants\n")
        f.write(f"{indent}{const_decl.format(name='numberOfNodes', value=nodeCount)}\n")
        f.write(f"{indent}{const_decl.format(name='totalNumFrames', value=frameCount)}\n")
        f.write(f"{indent}{const_decl.format(name='numMissingIDs', value=numMissingIDs)}\n")
        f.write(f"{indent}{const_decl.format(name='startingOffset', value=minId)}\n")
        f.write(f"\n{indent}//Explicilty defined in sensors.def constants\n")

        # Shared loop & eval logic — identical for both languages
        for define in globalDefines:
            raw = define.value.strip()
            try:
                val = eval_int_expr(raw) if isinstance(raw, str) else int(raw)
            except Exception:
                print("Warning, unable to eval as an expr")
                if isinstance(raw, str) and raw.startswith("0b"):
                    val = int(raw, 2)
                else:
                    raise ValueError(f"Non-numeric/unsupported expression for {define.name}: {raw!r}")

            # Single place that switches syntax via const_decl
            f.write(f"{indent}{const_decl.format(name=define.name, value=int(val))}\t\t// {raw}\n")
        writeEnums(f, lang)
         # === Add missingIDs as a static final array (Java only) ===
        if lang_l == "java" and missingIDs:
            genNodeIDsJava(f, missingIDs, nodeCount, minId)

        f.write(pre_wrapper_close)
        f.close()