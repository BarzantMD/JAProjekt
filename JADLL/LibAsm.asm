.586
.model flat, stdcall

EXTERN printf:proc ; mo¿liwoœæ skorzystania z wypisywania informacji na ekran

.data
text DB "Hello world", 10, 0

.code
TestProc2 proc x: DWORD
	mov eax, x
	mov ebx, [eax]
	mov edx, [eax + 4]
	add ebx, edx
	mov [eax], ebx
	mov eax, 3
	call MyProc1
	ret
TestProc2 endp

MyProc1 proc x: DWORD, y: DWORD
	push OFFSET text
	CALL printf	
	ret
ET1:mul x
	neg y
	ret
MyProc1 endp

TestProc proc x: DWORD, y: DWORD
	mov eax, x
	mov ebx, [eax]
	inc ebx
	mov [eax], ebx
	ret
TestProc endp
end