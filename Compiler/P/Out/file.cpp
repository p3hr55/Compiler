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
	mov eax, 1
	sub eax, 1
	imul eax, 4
	add eax, 12
	add eax, ebp
	mov ebx, 3
	mov [eax], ebx
	mov eax, 2
	sub eax, 1
	imul eax, 4
	add eax, 12
	add eax, ebp
	mov ebx, 1
	mov [eax], ebx
	mov eax, 3
	sub eax, 1
	imul eax, 4
	add eax, 12
	add eax, ebp
	mov ebx, 4
	mov [eax], ebx
	mov eax, 4
	sub eax, 1
	imul eax, 4
	add eax, 12
	add eax, ebp
	mov ebx, 5
	mov [eax], ebx
	mov eax, 5
	sub eax, 1
	imul eax, 4
	add eax, 12
	add eax, ebp
	mov ebx, 2
	mov [eax], ebx
	mov eax, 6
	sub eax, 1
	imul eax, 4
	add eax, 12
	add eax, ebp
	mov ebx, 7
	neg ebx
	mov [eax], ebx
	mov eax, 7
	sub eax, 1
	imul eax, 4
	add eax, 12
	add eax, ebp
	mov ebx, 8
	mov [eax], ebx
	mov eax, 8
	sub eax, 1
	imul eax, 4
	add eax, 12
	add eax, ebp
	mov ebx, 99
	mov [eax], ebx
	mov eax, 9
	sub eax, 1
	imul eax, 4
	add eax, 12
	add eax, ebp
	mov ebx, 0
	mov [eax], ebx
	mov eax, 10
	sub eax, 1
	imul eax, 4
	add eax, 12
	add eax, ebp
	mov ebx, 5
	mov [eax], ebx
	mov eax, 1
	mov [ebp + 0], eax

top_while_1:
	mov eax, [ebp + 0]
	mov ebx, 11
	cmp eax, ebx

	jge end_while_1
begin_while_1:
	mov eax, 2
	mov [ebp + 4], eax

top_while_2:
	mov eax, [ebp + 4]
	mov ebx, 12
	mov ecx, [ebp + 0]
	sub ebx, ecx
	cmp eax, ebx

	jge end_while_2
begin_while_2:
	mov eax, [ebp + 4]
	sub eax, 1
	imul eax, 4
	add eax, 12
	mov eax, [ebp + eax]
	mov ebx, [ebp + 4]
	mov ecx, 1
	sub ebx, ecx
	sub ebx, 1
	imul ebx, 4
	add ebx, 12
	mov ebx, [ebp + ebx]
	cmp eax, ebx

	jl if_begin_1

or_1:
	jmp else_1

if_begin_1:
	mov eax, [ebp + 4]
	sub eax, 1
	imul eax, 4
	add eax, 12
	mov eax, [ebp + eax]
	mov [ebp + 8], eax

	mov eax, [ebp + 4]
	sub eax, 1
	imul eax, 4
	add eax, 12
	add eax, ebp
	mov ebx, [ebp + 4]
	mov ecx, 1
	sub ebx, ecx
	sub ebx, 1
	imul ebx, 4
	add ebx, 12
	mov ebx, [ebp + ebx]
	mov [eax], ebx
	mov eax, [ebp + 4]
	mov ebx, 1
	sub eax, ebx
	sub eax, 1
	imul eax, 4
	add eax, 12
	add eax, ebp
	mov ebx, [ebp + 8]
	mov [eax], ebx
	jmp end_if_1

else_1:
end_if_1:
	mov eax, [ebp + 4]
	mov ebx, 1
	add eax, ebx
	mov [ebp + 4], eax

	jmp top_while_2
end_while_2:
	mov eax, [ebp + 0]
	mov ebx, 1
	add eax, ebx
	mov [ebp + 0], eax

	jmp top_while_1
end_while_1:
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
