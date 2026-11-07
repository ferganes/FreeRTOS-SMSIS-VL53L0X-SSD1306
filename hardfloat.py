# hardfloat.py
Import("env")

fpu_flags = [
    "-mfloat-abi=hard",
    "-mfpu=fpv4-sp-d16",
]

env.Append(
    CCFLAGS=fpu_flags,    # Для компиляции C/C++ (включая встроенный ассемблер)
    LINKFLAGS=fpu_flags,  # Для линковки
    ASFLAGS=fpu_flags,    # Для отдельных .s файлов
)