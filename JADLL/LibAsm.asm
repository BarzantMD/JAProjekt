.686
.model flat, stdcall

;EXTERN printf:proc ; mo¿liwoœæ skorzystania z wypisywania informacji na ekran
printf PROTO C :VARARG

.data
srcDataPointer DD 0; // wskaŸnik na dane Ÿród³owe
ALPHABET_POINTER DD 0 ; wskaŸnik na alfabet
ALPHABET_SIZE DB 0; ;iloœæ s³ów w alfabecie

.code
; ============================================================================;
; Procedura porównuje dwa ci¹gi bajtów i zwraca wynik w rejestrze eax.
; eax = 0 => ci¹gi s¹ takie same
; eax != 0 => ci¹gi s¹ ró¿ne
; ============================================================================;
CompareBytes PROC arg1: DWORD, arg2: DWORD, len: DWORD
	CLD ; inkrementujemy ESI i EDI
	MOV esi, arg1
	MOV edi, arg2
	MOV ecx, len ; za³aduj iloœæ bajtów do licznika

	; porównywanie kolejnych bajtów a¿ do napotkania ró¿nicy b¹dŸ wyzerowania licznika
cmpLoop:
	CMPSB
	LOOPE cmpLoop
	MOV eax, 1 ; domyœlne zwrócenie, ¿e ci¹gi s¹ ró¿ne
	MOV ebx, 0
	CMOVE eax, ebx ; jeœli s¹ takie same, to zwrócenie 0

	RET
CompareBytes ENDP

; ============================================================================;
TestProc proc var: DWORD
	INVOKE CompareBytes, OFFSET text, OFFSET text2, 5
	CMP eax, 0 ; sprawdzenie czy s¹ takie same
	JE rowne
	INVOKE printf, OFFSET nierowneMsg
	JMP koniec
rowne:
	INVOKE printf, OFFSET rowneMsg
koniec:
	RET
TestProc endp

; ============================================================================;
; Procedura odpowiedzialna za wyci¹gniêcie odpowiednich wskaŸników
; ze struktury argumentu do odpowiednich pól lokalnych danych.
; ============================================================================;
ParseParameters PROC arg: DWORD


ParseParameters ENDP

; ============================================================================;
; 
; ============================================================================;


; ============================================================================;
; G³ówna procedura odpowiedzialna za kompresjê danych.
; Otrzymuje wskaŸnik na strukturê CompressParamsAsm
; ============================================================================;
CompressAsm PROC params: DWORD
	MOV edi, params
	MOV eax, [edi + 20]
	ADD eax, 10
	MOV [edi + 20], eax
	RET
CompressAsm ENDP



; ============================================================================;
; 
; ============================================================================;

; ============================================================================;
; 
; ============================================================================;


end