.686
.xmm
.model flat, stdcall

;printf PROTO C :VARARG ; mo�liwo�� wypisywania na ekran informacji

.code
; ============================================================================;
; Struktura lokalnych danych dla u�atwienia przekazywania wielu zmiennych
; do procedur.
; ============================================================================;
PROC_DATA_STR STRUCT
	DICT_POINTER DWORD ? ; wska�nik na obszar pami�ci dla s�ownika
	DICT_END_POINTER DWORD ? ; wska�nik na pierwszy wolny bajt s�ownika 
	DICT_MAX_SIZE DWORD ? ; maksymalny rozmiar s�ownika
	DICT_CUR_SIZE DWORD ? ; aktualny rozmiar s�ownika
	DICT_COUNT DWORD ? ; ilo�� element�w w s�owniku

	BLOCK_COUNT DWORD ? ; ilo�� blok�w
	CUR_BLOCK_POINTER DWORD ? ; wska�nik na aktualnie zapisywany blok

	COMPRESSED_DATA_SIZE DWORD ? ; ilo�� skompresowanych danych w bloku (kod�w)
	SUMMARY_COMPRESSED_DATA_SIZE DWORD ? ; og�lnie ilo�� skompresowanych danych (kod�w)

	TEMP_CODEWORD_POINTER DWORD ? ; tymczasowo rozpoznany ci�g (zaczynaj�c od dw�ch bajt�w okre�laj�cych jego d�ugo��)
	LAST_CODE DWORD ? ; kod ostatnio rozpoznanego ci�gu
	TEMP_CODE DWORD ? ; tymczasowy kod rozpozanego ci�gu
PROC_DATA_STR ENDS

; ============================================================================;
; Procedura dodaje nowe s�owo kodowe do s�ownika.
; Przyjmowane argumenty:
; codeword - wska�nik na s�owo kodowe
; len - d�ugo�� s�owa kodowego
; ============================================================================;
dictAddCodeword PROC codeword: DWORD, len: WORD, PROC_DATA_PTR: DWORD
	; zachowanie rejestr�w
	PUSHAD

	; ustanowienie edx jako wska�nika na struktur� PROC_DATA_STR
	ASSUME edx : PTR PROC_DATA_STR
	MOV edx, PROC_DATA_PTR

	; ustawienie licznika (d�ugo�� s�owa kodowego)
	MOV ecx, 0
	MOV cx, len

	; zwi�kszenie rozmiaru s�ownika
	ADD [edx].DICT_CUR_SIZE, ecx

	; zwi�kszenie liczby element�w w s�owniku
	ADD [edx].DICT_COUNT, 1

	; ustalamy �r�d�owy adres odczytu danych
	MOV esi, codeword

	; ustawiamy docelowy adres zapisu danych
	MOV edi, [edx].DICT_END_POINTER 

	; zapisanie d�ugo�ci s�owa kodowego
	MOV [edi], cx 

	; ustawienie si� na miejsce zapisu s�owa kodowego
	ADD edi, 2 
	CLD
	; skopiowanie s�owa kodowego do s�ownika
	REP MOVSB 

	; ustalenie nowego ko�ca s�ownika
	MOV [edx].DICT_END_POINTER, edi 

	; odtworzenie rejestr�w
	POPAD
	RET
dictAddCodeword ENDP

; ============================================================================;
; Procedura inicjuje s�ownik alfabetem.
; ============================================================================;
dictInitAlphabet PROC PROC_DATA_PTR: DWORD
	; zachowanie rejestr�w
	PUSHAD

	; odczytanie ilo�ci znak�w w alfabecie
	MOV ecx, [ebx + 32] 
	; wska�nik na alfabet
	MOV esi, [ebx + 28] 

alphabetLoop:
	; dodaj pojedynczy znak alfabetu do s�ownika
	INVOKE dictAddCodeword, esi, 1, PROC_DATA_PTR
	ADD esi, 1
	LOOP alphabetLoop

	POPAD
	RET
dictInitAlphabet ENDP

; ============================================================================;
; Procedura inicjuj�ca zmienne DICT_POINTER oraz DICT_END_POINTER
; ============================================================================;
dictInit PROC PROC_DATA_PTR: DWORD
	; zachowanie rejestr�w
	PUSHAD

	; ustanowienie edx jako wska�nika na struktur� PROC_DATA_STR
	ASSUME edx : PTR PROC_DATA_STR
	MOV edx, PROC_DATA_PTR

	; ustawiamy wska�nik na miejsce dla s�ownika
	MOV eax, [ebx + 20] 
	MOV [edx].DICT_POINTER, eax

	; pierwszym wolnym bajtem jest pocz�tek
	MOV [edx].DICT_END_POINTER, eax 

	; ustawiamy maksymalny rozmiar danych w s�owniku
	MOV eax, [ebx + 24] 
	MOV [edx].DICT_MAX_SIZE, eax
	MOV [edx].DICT_CUR_SIZE, 0
	MOV [edx].DICT_COUNT, 0

	; przywr�cenie rejestr�w
	POPAD
	RET
dictInit ENDP

; ============================================================================;
; Procedura por�wnuj�ca dwa ci�gi tradycyjnym sposobem.
; Ci�gi s� przekazywane przez dwa rejestry wskazuj�ce na ci�gi,
; oraz d�ugo�� tych ci�g�w w rejestrze ecx.
; ============================================================================;
compareString PROC
	; zapami�tanie adresu �r�d�owego do kolejnego por�wnania (z kolejnym elementem)
	PUSH esi 
	
	;p�tla por�wnania danego elementu
cmpLoop:
	CMPSB
	LOOPE cmpLoop
	; przywr�cenie pierwotnego esi dla por�wnania z kolejnym elementem
	POP esi  
	RET
compareString ENDP

; ============================================================================;
; Procedura por�wnuj�ca dwa ci�gi wykorzystuj�c je�li to mo�liwe rejestry
; technologii SSE (XMM).
; Ci�gi s� przekazywane przez dwa rejestry wskazuj�ce na ci�gi,
; oraz d�ugo�� tych ci�g�w w rejestrze ecx.
; ============================================================================;
compareStringSIMD128 PROC
	; zapami�tanie adresu �r�d�owego do kolejnego por�wnania (z kolejnym elementem)
	PUSH esi
	; z rejestru eax b�dziemy korzystali do maski bit�w z rejestru xmm 
	PUSH eax
	
	;p�tla por�wnania danego elementu
cmpSIMD:
	; sprawdzamy czy pozosta�y ci�g do por�wnania jest d�u�szy ni� 16 bajt�w
	CMP ecx, 16
	; je�li mniejszy, to skaczemy do tradycyjnego por�wnania
	JNGE cmpLoopEnter
	; por�wnanie 16 bajt�w przy pomocy SIMD
	; za�adowanie 16B jednego ci�gu do xmm0
	MOVDQU xmm0, OWORD PTR [esi]
	; za�adowanie 16B drugiego ci�gu do xmm1
	MOVDQU xmm1, OWORD PTR [edi]
	; por�wnanie 16B
	PCMPEQB xmm0, xmm1
	; wyci�gni�cie maski bitowej
	PMOVMSKB eax, xmm0
	; warto�� 0FFFFh oznacza, �e 16B by�o r�wnych sobie
	CMP eax, 0FFFFh
	; je�li nie by�o zgodno�ci, ko�czymy por�wnanie i wychodzimy z por�wnania
	JNE exitCompare
	
	; modyfikujemy odpowiednio rejestry z ci�gami
	ADD edi, 16
	ADD esi, 16
	SUB ecx, 16
	; pr�bujemy dalej por�wnywa� za pomoc� SIMD
	JMP cmpSIMD


cmpLoopEnter:
	; sprawdzenie czy jest jeszcze co� do por�wnania
	CMP ecx, 0
	JE exitCompare

	; por�wnanie pozosta�ych bajt�w zwyk�ym sposobem
cmpLoop:
	CMPSB
	LOOPE cmpLoop

exitCompare:
	; przywr�cenie eax
	POP eax
	; przywr�cenie pierwotnego esi dla por�wnania z kolejnym elementem
	POP esi  
	RET
compareStringSIMD128 ENDP

; ============================================================================;
; Procedura por�wnuj�ca dwa ci�gi wykorzystuj�c je�li to mo�liwe rejestry
; technologii SSE (MMX).
; Ci�gi s� przekazywane przez dwa rejestry wskazuj�ce na ci�gi,
; oraz d�ugo�� tych ci�g�w w rejestrze ecx.
; ============================================================================;
compareStringSIMD64 PROC
	; zapami�tanie adresu �r�d�owego do kolejnego por�wnania (z kolejnym elementem)
	PUSH esi
	; z rejestru eax b�dziemy korzystali do maski bit�w z rejestru xmm 
	PUSH eax
	
	;p�tla por�wnania danego elementu
cmpSIMD:
	; sprawdzamy czy pozosta�y ci�g do por�wnania jest d�u�szy ni� 16 bajt�w
	CMP ecx, 8
	; je�li mniejszy, to skaczemy do tradycyjnego por�wnania
	JNGE cmpLoopEnter
	; por�wnanie 8 bajt�w przy pomocy SIMD
	; za�adowanie 8B jednego ci�gu do xmm0
	MOVQ mm0, QWORD PTR [esi]
	; za�adowanie 8B drugiego ci�gu do xmm1
	MOVQ mm1, QWORD PTR [edi]
	; por�wnanie 16B
	PCMPEQB mm0, mm1
	; wyci�gni�cie maski bitowej
	PMOVMSKB eax, mm0
	; warto�� 0FFh oznacza, �e 16B by�o r�wnych sobie
	CMP eax, 0FFh
	; je�li nie by�o zgodno�ci, ko�czymy por�wnanie i wychodzimy z por�wnania
	JNE exitCompare
	
	; modyfikujemy odpowiednio rejestry z ci�gami
	ADD edi, 8
	ADD esi, 8
	SUB ecx, 8
	; pr�bujemy dalej por�wnywa� za pomoc� SIMD
	JMP cmpSIMD


cmpLoopEnter:
	; sprawdzenie czy jest jeszcze co� do por�wnania
	CMP ecx, 0
	JE exitCompare

	; por�wnanie pozosta�ych bajt�w zwyk�ym sposobem
cmpLoop:
	CMPSB
	LOOPE cmpLoop

exitCompare:
	; przywr�cenie eax
	POP eax
	; przywr�cenie pierwotnego esi dla por�wnania z kolejnym elementem
	POP esi  
	RET
compareStringSIMD64 ENDP

; ============================================================================;
; Procedura zwraca kod podanego ci�gu kodowego (przekazanego przez rejestry
; esi (adres) oraz edx (d�ugo��)
; Zwraca w TEMP_CODEWORD_POINTER adres na dany ci�g w s�owniku (��cznie z jego
; rozmiarem) oraz jego kod poprzez zmienn� TEMP_CODE.
; Je�li nie ma takiego ci�gu w s�owniku, to TEMP_CODEWORD_POINTER = 0
; ============================================================================;
dictGetCodewordId PROC PROC_DATA_PTR: DWORD
	PUSHAD

	; ustanowienie eax jako wska�nika na struktur� PROC_DATA_STR
	ASSUME eax : PTR PROC_DATA_STR
	MOV eax, PROC_DATA_PTR

	MOV edi, [eax].DICT_POINTER

	; aktualny numer (kod) elementu w s�owniku
	MOV ebx, 0 

	; zerujemy ecx - b�dzie s�u�y� za d�ugo�� aktualnego elementu w s�owniku (jego dolne 2 bajty)
	MOV ecx, 0 
	
	; p�tla przeszukiwania kolejnych element�w s�ownika
	CLD
searchLoop:
	; zapami�tanie adresu na wypadek znalezienia
	PUSH edi 

	; pobieramy d�ugo�� elementu
	MOV cx, [edi] 
	ADD edi, 2

	; sprawdzamy, czy d�ugo�ci s� r�wne
	CMP ecx, edx 

	; nie ma sensu por�wnywa� ci�g�w o r�nych d�ugo�ciach - przeskoczenie do kolejnego elementu
	JNE skip 

	; por�wnanie ci�g�w (zwrot wyniku przez ustawienie flagi
	INVOKE compareString

	; je�li ca�y ci�g si� zgadza
	JE equal 

skip:
	; pomini�cie niepor�wnanych bajt�w
	; przesuni�cie edi o d�ugo�� niepor�wnanych danych (lub nawet o ca�y element)
	ADD edi, ecx
next:
	; przechodzimy do kolejnego elementu
	ADD ebx, 1
	; usuwamy zapami�tany adres ze stosu
	ADD esp, 4
	; sprawdzamy czy nie przeszukali�my ju� wszystkich element�w w s�owniku
	CMP ebx, [eax].DICT_COUNT 
	JNZ searchLoop

	; je�li nie znaleziono elementu w s�owniku
	; zwr�cenie zerowego wska�nika (nie znaleziono elementu)
	MOV [eax].TEMP_CODEWORD_POINTER, 0 

	JMP exit

equal:
	; na stosie zapami�tano adres na wypadek znalezienia ci�gu
	POP [eax].TEMP_CODEWORD_POINTER
	; ebx wskazuje na kod znalezionego ci�gu
	MOV [eax].TEMP_CODE, ebx 

exit:
	POPAD
	; dodatkowe zwr�cenie wska�nika przez eax
	PUSH [eax].TEMP_CODEWORD_POINTER
	POP eax
	RET
dictGetCodewordId ENDP

; ============================================================================;
; G��wna procedura odpowiedzialna za kompresj� danych.
; Otrzymuje wska�nik na struktur� CompressParamsAsm
; Funkcje rejestr�w:
; esi - wska�nik na aktualnie analizowany ci�g �r�d�owy
; edi - wska�nik na kolejn� pozycj� danych skompresowanych do zapisania
;       kodu kolejnego s�owa kodowego
; ecx - ilo�� bajt�w pozosta�ych w danych �r�d�owych
; edx - d�ugo�� aktualnie analizowanego ci�gu
; ============================================================================;
CompressThreadAsm PROC params: DWORD
	; struktura przechowuj�ca zmienne potrzebne do wykonania kompresji
	LOCAL PROC_DATA : PROC_DATA_STR 

	; ebx dla kolejnych dw�ch procedur b�dzie wskazywa� struktur� params
	MOV ebx, params 

	; innicjalizacja zmiennych lokalnych
	MOV PROC_DATA.BLOCK_COUNT, 1
	MOV PROC_DATA.COMPRESSED_DATA_SIZE, 0
	MOV PROC_DATA.SUMMARY_COMPRESSED_DATA_SIZE, 0

	; inicjalizacja zmiennych s�ownika
	INVOKE dictInit, ADDR PROC_DATA 
	; inicjalizacja s�ownika alfabetem
	INVOKE dictInitAlphabet, ADDR PROC_DATA

	; adres danych wej�ciowych
	MOV esi, [ebx + 0] 

	; rozmiar danych wej�ciowych (ilo�� bajt�w)
	MOV ecx, [ebx + 4] 

	; adres danych wyj�ciowych (skompresowanych)
	MOV edi, [ebx + 8] 
	MOV PROC_DATA.CUR_BLOCK_POINTER, edi

	; 4 bajty rezerwujemy na rozmiar bloku
	ADD edi, 4 

	; pocz�tkowa ilo�� wczytanych znak�w - 0
	MOV edx, 0 

	; g��wna p�tla wczytuj�ca kolejne znaki ze �r�d�a
dataLoop:
	; wczytaj kolejny znak
	ADD edx, 1 
	; sprawdzamy czy jest ci�g w s�owniku - przekazanie argument�w przez esi oraz edx
	INVOKE dictGetCodewordId, ADDR PROC_DATA
	; sprawdzamy czy jest w s�owniku
	CMP eax, 0 
	; je�eli ci�g jest w s�owniku
	JNZ next 

	; ci�gu nie ma w s�owniku

	; 1 wypisz kod zwi�zany z c i zaktualizuj rozmiar danych skompresowanych
	MOV	eax, PROC_DATA.LAST_CODE
	MOV [edi], ax

	; przesuwamy si� o 2 bajty, na kolejn� pozycj� do zapisu kodu
	ADD edi, 2 
	ADD PROC_DATA.COMPRESSED_DATA_SIZE, 1

	; 2 dodaj przed�u�ony ostatnio nierozpoznany ci�g do s�ownika
	INVOKE dictAddCodeword, esi, dx, ADDR PROC_DATA

	; 3 prefiksem do kolejnego ci�gu, jest ostatni znak z dodanego ci�gu do s�ownika
	; dodajemy d�ugo�� ostatniego ci�gu
	ADD esi, edx 
	; cofamy si� o jeden znak
	SUB esi, 1 
	; ustawiamy d�ugo�� nowego analizowanego ci�gu na 1
	MOV edx, 1 

	; analiza czy s�ownik osi�gn�� limit rozmiaru (DICT_MAX_SIZE)
	MOV eax, PROC_DATA.DICT_CUR_SIZE
	CMP PROC_DATA.DICT_MAX_SIZE, eax
	; s�ownik jeszcze nie przekroczy� dozwolonego rozmiaru - sprawdzenie drugiego kryterium
	JG check
	; s�ownik przekroczy� dozwolony rozmiar - tworzymy nowy
	JMP newDict

check:
	; analiza czy s�ownik osi�gn�� limit element�w (na sztywno ustawiony na 16384 element�w)
	CMP PROC_DATA.DICT_COUNT, 16384
	; nie osi�gn�� limitu element�w
	JNGE ok

newDict:
	; s�ownik osi�gn�� limit, trzeba wyczy�ci�, utworzy� nowy, i rozpocz�� nowy blok
	; 1. inicjalizacja nowego s�ownika
	; ebx dla kolejnych dw�ch procedur b�dzie wskazywa� struktur� params
	MOV ebx, params 
	; inicjalizacja zmiennych s�ownika
	INVOKE dictInit, ADDR PROC_DATA 
	; inicjalizacja s�ownika alfabetem
	INVOKE dictInitAlphabet, ADDR PROC_DATA

	; 2. zapisanie rozmiaru bloku, dodanie do sumarycznego rozmiaru
	; pobranie adresu pocz�tku bloku
	MOV ebx, PROC_DATA.CUR_BLOCK_POINTER 
	MOV eax, PROC_DATA.COMPRESSED_DATA_SIZE
	MOV [ebx], eax
	ADD PROC_DATA.SUMMARY_COMPRESSED_DATA_SIZE, eax

	; 3. zwi�kszenie numeracji ilo�ci blok�w
	ADD PROC_DATA.BLOCK_COUNT, 1
	
	; 4. ustawienie nowego bloku
	; otrzymanie ilo�ci bajt�w danych skompresowanych
	ADD eax, eax 
	; dodanie 4 bajt�w rozmiaru z pocz�tku bloku
	ADD eax, 4 
	; ustalenie nowego pocz�tku bloku
	ADD PROC_DATA.CUR_BLOCK_POINTER, eax 
	; nowe miejsce zapisu kod�w
	MOV edi, PROC_DATA.CUR_BLOCK_POINTER 
	; zarezerwowanie 4 bajt�w na rozmiar nowego bloku
	ADD edi, 4 
	; wyzerowanie aktualnego rozmiaru danych skompresowanych
	MOV PROC_DATA.COMPRESSED_DATA_SIZE, 0 

ok:
	; aktualizujemy ostatnio rozpoznany ci�g
	; musi by� w s�owniku, poniewa� jest to pojedynczy znak alfabetu
	INVOKE dictGetCodewordId, ADDR PROC_DATA

next:
	MOV eax, PROC_DATA.TEMP_CODE
	; zapami�taj ostatnio rozpoznany kod
	MOV PROC_DATA.LAST_CODE, eax 

	DEC ecx
	JNZ dataLoop

	; na ko�cu wypisujemy kod zwi�zany z ostatnio rozpoznanym symbolem i aktualizujemy rozmiar danych skompresowanych
	MOV	eax, PROC_DATA.LAST_CODE
	MOV [edi], ax
	ADD PROC_DATA.COMPRESSED_DATA_SIZE, 1
	MOV eax, PROC_DATA.COMPRESSED_DATA_SIZE
	ADD PROC_DATA.SUMMARY_COMPRESSED_DATA_SIZE, eax

	; zapisujemy rozmiar ostatniego bloku
	MOV eax, PROC_DATA.COMPRESSED_DATA_SIZE
	MOV edi, PROC_DATA.CUR_BLOCK_POINTER
	MOV [edi], eax

	; aktualizujemy dane zwrotne w parametrach kompresji
	MOV ebx, params
	MOV eax, PROC_DATA.SUMMARY_COMPRESSED_DATA_SIZE
	MOV [ebx + 12], eax

	MOV eax, PROC_DATA.BLOCK_COUNT
	MOV [ebx + 16], eax

	RET
CompressThreadAsm ENDP


end