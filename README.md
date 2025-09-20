
# Monic Polynomial Solver in C

## Overview
This C program reads a JSON input describing polynomial roots in various bases, converts the roots to decimal, and constructs the **monic polynomial** with those roots. It prints the first `k` roots in decimal and the polynomial coefficients from **highest degree to constant term**.

---

## Input Format
Input is provided via **standard input (STDIN)** in JSON format. Example:

```json
{
  "keys": {
    "n": 4,
    "k": 3
  },
  "1": {"base":"10","value":"4"},
  "2": {"base":"2","value":"111"},
  "3": {"base":"10","value":"12"},
  "6": {"base":"4","value":"213"}
}


n = total number of roots provided

k = number of roots used to form the polynomial

"base" = base of the root number

"value" = root value as a string

Notes: Only the first k roots are used.

Output Format

The program prints:

k
roots_decimal_first_k
<root1>
<root2>
...
degree
<polynomial_degree>
coefficients_high_to_low
<c_high>
<c_next>
...
<c0>


Example Output:

3
roots_decimal_first_k
4
7
12
degree
3
coefficients_high_to_low
1
-23
160
-336

How to Run
Online

Open an online C compiler (OnlineGDB, JDoodle, Ideone).

Paste polynomial.c into the editor.

In STDIN/Text input, paste the JSON input.

Click Run.

Locally (Windows)

Install MinGW or use WSL (Ubuntu).

Save code as polynomial.c and JSON input as input.json.

Compile:

gcc -std=c11 -O2 -Wall -Wextra polynomial.c -o poly.exe


Run:

poly.exe < input.json

Notes / Limitations

Program uses int64_t → roots must fit in 64-bit signed integers.

Extremely large roots (like in second test case) may overflow.

Input JSON must strictly follow the schema above.

Author

Rajesh Sandaka


---
