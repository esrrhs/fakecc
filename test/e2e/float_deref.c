// expect: 5
// Float loaded through a pointer (IR_LOAD_PTR in EX_DEREF).  Same root cause
// as float_struct_member: the load's value_is_float metadata was never set,
// so the bits were loaded into a GPR instead of an XMM register.
package main;
int main(void) {
    float a = 2.5f;
    float *p = &a;
    float b = *p * 2.0f;
    return (int)b;
}
