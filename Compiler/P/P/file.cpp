#include <iostream>
using namespace std;
char DataSegment[65536];

int main() {
__asm {
	push eax
	push ebx
	push ecx
	push edx
	push ebp
	push edi
	push esi
	push esp

	lea eax, DataSegment
	mov ebp, eax
	jmp kmain

kmain:
	mov eax, 3
	mov ebx, 3
	imul eax, ebx
	mov ebx, 3
	imul eax, ebx
	mov [ebp + 0], eax

	pop esp
	pop esi
	pop edi
	pop ebp
	pop edx
	pop ecx
	pop ebx
	pop eax

}

return 0;
}
