.686
.model flat, stdcall

;EXTERN printf:proc ; mo�liwo�� skorzystania z wypisywania informacji na ekran
printf PROTO C :VARARG

.data
srcDataPointer DD 0; // wska�nik na dane �r�d�owe
ALPHABET_POINTER DD 0 ; wska�nik na alfabet
ALPHABET_SIZE DB 0; ;ilo�� s��w w alfabecie

.code
; ============================================================================;
; Procedura por�wnuje dwa ci�gi bajt�w i zwraca wynik w rejestrze eax.
; eax = 0 => ci�gi s� takie same
; eax != 0 => ci�gi s� r�ne
; ============================================================================;
CompareBytes PROC arg1: DWORD, arg2: DWORD, len: DWORD
	CLD ; inkrementujemy ESI i EDI
	MOV esi, arg1
	MOV edi, arg2
	MOV ecx, len ; za�aduj ilo�� bajt�w do licznika

	; por�wnywanie kolejnych bajt�w a� do napotkania r�nicy b�d� wyzerowania licznika
cmpLoop:
	CMPSB
	LOOPE cmpLoop
	MOV eax, 1 ; domy�lne zwr�cenie, �e ci�gi s� r�ne
	MOV ebx, 0
	CMOVE eax, ebx ; je�li s� takie same, to zwr�cenie 0

	RET
CompareBytes ENDP

; ============================================================================;
TestProc proc var: DWORD
	INVOKE CompareBytes, OFFSET text, OFFSET text2, 5
	CMP eax, 0 ; sprawdzenie czy s� takie same
	JE rowne
	INVOKE printf, OFFSET nierowneMsg
	JMP koniec
rowne:
	INVOKE printf, OFFSET rowneMsg
koniec:
	RET
TestProc endp

; ============================================================================;
; Procedura odpowiedzialna za wyci�gni�cie odpowiednich wska�nik�w
; ze struktury argumentu do odpowiednich p�l lokalnych danych.
; ============================================================================;
ParseParameters PROC arg: DWORD


ParseParameters ENDP

; ============================================================================;
; 
; ============================================================================;


; ============================================================================;
; G��wna procedura odpowiedzialna za kompresj� danych.
; Otrzymuje wska�nik na struktur� CompressParamsAsm
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