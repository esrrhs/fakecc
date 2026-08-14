// expect: 0
// Stress-test: many φ variables at the same loop header with calls
package main;


import runtime;
int test_many_phi(int n, int a0, int a1, int a2, int a3, int a4,
                  int a5, int a6, int a7, int a8, int a9) {
    int v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15;
    int flag;
    int i;
    v0=a0; v1=a1; v2=a2; v3=a3; v4=a4; v5=a5;
    v6=a6; v7=a7; v8=a8; v9=a9;
    v10=10; v11=11; v12=12; v13=13; v14=14; v15=15;
    flag=0; i=0;
L:
    if(i>=n) goto done;
    runtime.puts("lp");
    if(flag==0) flag=i+100;
    v0=v0+1; v1=v1+1; v2=v2+1; v3=v3+1;
    v4=v4+1; v5=v5+1; v6=v6+1; v7=v7+1;
    v8=v8+1; v9=v9+1; v10=v10+1; v11=v11+1;
    v12=v12+1; v13=v13+1; v14=v14+1; v15=v15+1;
    i=i+1;
    goto L;
done:
    /* flag is still 0 exactly when the loop body never ran; -1 marks that. */
    if(flag==0) flag=-1;
    if(n<=0){ if(flag!=-1) return 1; }
    else { if(flag!=100) return 2; }
    if(v0!=a0+n) return 10;
    if(v1!=a1+n) return 11;
    if(v2!=a2+n) return 12;
    if(v3!=a3+n) return 13;
    if(v4!=a4+n) return 14;
    if(v5!=a5+n) return 15;
    if(v6!=a6+n) return 16;
    if(v7!=a7+n) return 17;
    if(v8!=a8+n) return 18;
    if(v9!=a9+n) return 19;
    if(v10!=10+n) return 20;
    if(v11!=11+n) return 21;
    if(v12!=12+n) return 22;
    if(v13!=13+n) return 23;
    if(v14!=14+n) return 24;
    if(v15!=15+n) return 25;
    return 0;
}

int main(void) {
    int r;
    r = test_many_phi(0,1,2,3,4,5,6,7,8,9,10);
    if(r!=0) return r;
    r = test_many_phi(15,1,2,3,4,5,6,7,8,9,10);
    if(r!=0) return r;
    return 0;
}
