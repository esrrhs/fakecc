// expect: 5
// Float loaded from an array element (IR_LOAD_PTR in EX_INDEX).  Same root
// cause as float_struct_member: the load's value_is_float metadata was never
// set, so the bits were loaded into a GPR instead of an XMM register.
package main;
int main(void) {
    float a[2];
    a[0] = 2.5f;
    a[1] = a[0];
    return (int)(a[1] * 2.0f);
}
