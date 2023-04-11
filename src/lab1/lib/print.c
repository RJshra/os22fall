#include "print.h"
#include "sbi.h"
void puts(char *s) {
    for(int i=0;s[i]!='\0';i++){
        sbi_ecall(0x1,0x0,s[i],0,0,0,0,0);
    }
    
}

int quickpow(int a,int n){
    int ans = 1;
    while(n>0){
        if(n&1>0)        
            ans *= a;  
        a *= a;       
        n >>= 1;       
    }
    return ans;
}

void puti(int x) {
    if(x<0){
        sbi_ecall(0x1,0x0,0x2d,0,0,0,0,0);
        x*=-1;
    }
    if(x==0){
        sbi_ecall(0x1,0x0,x,0,0,0,0,0);
    }
    else{

        int n=x,bit=0;
          while(n>0){
             n/=10;
             bit++;
         }
        while(bit>0){
            sbi_ecall(0x1,0x0,(x/quickpow(10,bit-1))+0x30,0,0,0,0,0);
            x=x%quickpow(10,bit-1);
            bit--;
        }
    }
    

}
