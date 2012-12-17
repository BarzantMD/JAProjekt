.686
.xmm
.model flat, stdcall

;printf PROTO C :VARARG ; mo¿liwoœæ wypisywania na ekran informacji

.code
; ============================================================================;
; Struktura lokalnych danych dla u³atwienia przekazywania wielu zmiennych
; do procedur.
; ============================================================================;
PROC_DATA_STR STRUCT
	DICT_POINTER DWORD ? ; wskaŸnik na obszar pamiêci dla s³ownika
	DICT_END_POINTER DWORD ? ; wskaŸnik na pierwszy wolny bajt s³ownika 
	DICT_MAX_SIZE DWORD ? ; maksymalny rozmiar s³ownika
	DICT_CUR_SIZE DWORD ? ; aktualny rozmiar s³ownika
	DICT_COUNT DWORD ? ; iloœæ elementów w s³owniku

	BLOCK_COUNT DWORD ? ; iloœæ bloków
	CUR_BLOCK_POINTER DWORD ? ; wskaŸnik na aktualnie zapisywany blok

	COMPRESSED_DATA_SIZE DWORD ? ; iloœæ skompresowanych danych w bloku (kodów)
	SUMMARY_COMPRESSED_DATA_SIZE DWORD ? ; ogólnie iloœæ skompresowanych danych (kodów)

	TEMP_CODEWORD_POINTER DWORD ? ; tymczasowo rozpoznany ci¹g (zaczynaj¹c od dwóch bajtów okreœlaj¹cych jego d³ugoœæ)
	LAST_CODE DWORD ? ; kod ostatnio rozpoznanego ci¹gu
	TEMP_CODE DWORD ? ; tymczasowy kod rozpozanego ci¹gu
PROC_DATA_STR ENDS

; ============================================================================;
; Procedura dodaje nowe s³owo kodowe do s³ownika.
; Przyjmowane argumenty:
; codeword - wskaŸnik na s³owo kodowe
; len - d³ugoœæ s³owa kodowego
; ============================================================================;
dictAddCodeword PROC codeword: DWORD, len: WORD, PROC_DATA_PTR: DWORD
	; zachowanie rejestrów
	PUSHAD

	; ustanowienie edx jako wskaŸnika na strukturê PROC_DATA_STR
	ASSUME edx : PTR PROC_DATA_STR
	MOV edx, PROC_DATA_PTR

	; ustawienie licznika (d³ugoœæ s³owa kodowego)
	MOV ecx, 0
	MOV cx, len

	; zwiêkszenie rozmiaru s³ownika
	ADD [edx].DICT_CUR_SIZE, ecx

	; zwiêkszenie liczby elementów w s³owniku
	ADD [edx].DICT_COUNT, 1

	; ustalamy Ÿród³owy adres odczytu danych
	MOV esi, codeword

	; ustawiamy docelowy adres zapisu danych
	MOV edi, [edx].DICT_END_POINTER 

	; zapisanie d³ugoœci s³owa kodowego
	MOV [edi], cx 

	; ustawienie siê na miejsce zapisu s³owa kodowego
	ADD edi, 2 
	CLD
	; skopiowanie s³owa kodowego do s³ownika
	REP MOVSB 

	; ustalenie nowego koñca s³ownika
	MOV [edx].DICT_END_POINTER, edi 

	; odtworzenie rejestrów
	POPAD
	RET
dictAddCodeword ENDP

; ============================================================================;
; Procedura inicjuje s³ownik alfabetem.
; ============================================================================;
dictInitAlphabet PROC PROC_DATA_PTR: DWORD
	; zachowanie rejestrów
	PUSHAD

	; odczytanie iloœci znaków w alfabecie
	MOV ecx, [ebx + 32] 
	; wskaŸnik na alfabet
	MOV esi, [ebx + 28] 

alphabetLoop:
	; dodaj pojedynczy znak alfabetu do s³ownika
	INVOKE dictAddCodeword, esi, 1, PROC_DATA_PTR
	ADD esi, 1
	LOOP alphabetLoop

	POPAD
	RET
dictInitAlphabet ENDP

; ============================================================================;
; Procedura inicjuj¹ca zmienne DICT_POINTER oraz DICT_END_POINTER
; ============================================================================;
dictInit PROC PROC_DATA_PTR: DWORD
	; zachowanie rejestrów
	PUSHAD

	; ustanowienie edx jako wskaŸnika na strukturê PROC_DATA_STR
	ASSUME edx : PTR PROC_DATA_STR
	MOV edx, PROC_DATA_PTR

	; ustawiamy wskaŸnik na miejsce dla s³ownika
	MOV eax, [ebx + 20] 
	MOV [edx].DICT_POINTER, eax

	; pierwszym wolnym bajtem jest pocz¹tek
	MOV [edx].DICT_END_POINTER, eax 

	; ustawiamy maksymalny rozmiar danych w s³owniku
	MOV eax, [ebx + 24] 
	MOV [edx].DICT_MAX_SIZE, eax
	MOV [edx].DICT_CUR_SIZE, 0
	MOV [edx].DICT_COUNT, 0

	; przywrócenie rejestrów
	POPAD
	RET
dictInit ENDP

; ============================================================================;
; Procedura porównuj¹ca dwa ci¹gi tradycyjnym sposobem.
; Ci¹gi s¹ przekazywane przez dwa rejestry wskazuj¹ce na ci¹gi,
; oraz d³ugoœæ tych ci¹gów w rejestrze ecx.
; ============================================================================;
compareString PROC
	; zapamiêtanie adresu Ÿród³owego do kolejnego porównania (z kolejnym elementem)
	PUSH esi 
	
	;pêtla porównania danego elementu
cmpLoop:
	CMPSB
	LOOPE cmpLoop
	; przywrócenie pierwotnego esi dla porównania z kolejnym elementem
	POP esi  
	RET
compareString ENDP

; ============================================================================;
; Procedura porównuj¹ca dwa ci¹gi wykorzystuj¹c jeœli to mo¿liwe rejestry
; technologii SSE (XMM).
; Ci¹gi s¹ przekazywane przez dwa rejestry wskazuj¹ce na ci¹gi,
; oraz d³ugoœæ tych ci¹gów w rejestrze ecx.
; ============================================================================;
compareStringSIMD128 PROC
	; zapamiêtanie adresu Ÿród³owego do kolejnego porównania (z kolejnym elementem)
	PUSH esi
	; z rejestru eax bêdziemy korzystali do maski bitów z rejestru xmm 
	PUSH eax
	
	;pêtla porównania danego elementu
cmpSIMD:
	; sprawdzamy czy pozosta³y ci¹g do porównania jest d³u¿szy ni¿ 16 bajtów
	CMP ecx, 16
	; jeœli mniejszy, to skaczemy do tradycyjnego porównania
	JNGE cmpLoopEnter
	; porównanie 16 bajtów przy pomocy SIMD
	; za³adowanie 16B jednego ci¹gu do xmm0
	MOVDQU xmm0, OWORD PTR [esi]
	; za³adowanie 16B drugiego ci¹gu do xmm1
	MOVDQU xmm1, OWORD PTR [edi]
	; porównanie 16B
	PCMPEQB xmm0, xmm1
	; wyci¹gniêcie maski bitowej
	PMOVMSKB eax, xmm0
	; wartoœæ 0FFFFh oznacza, ¿e 16B by³o równych sobie
	CMP eax, 0FFFFh
	; jeœli nie by³o zgodnoœci, koñczymy porównanie i wychodzimy z porównania
	JNE exitCompare
	
	; modyfikujemy odpowiednio rejestry z ci¹gami
	ADD edi, 16
	ADD esi, 16
	SUB ecx, 16
	; próbujemy dalej porównywaæ za pomoc¹ SIMD
	JMP cmpSIMD


cmpLoopEnter:
	; sprawdzenie czy jest jeszcze coœ do porównania
	CMP ecx, 0
	JE exitCompare

	; porównanie pozosta³ych bajtów zwyk³ym sposobem
cmpLoop:
	CMPSB
	LOOPE cmpLoop

exitCompare:
	; przywrócenie eax
	POP eax
	; przywrócenie pierwotnego esi dla porównania z kolejnym elementem
	POP esi  
	RET
compareStringSIMD128 ENDP

; ============================================================================;
; Procedura porównuj¹ca dwa ci¹gi wykorzystuj¹c jeœli to mo¿liwe rejestry
; technologii SSE (MMX).
; Ci¹gi s¹ przekazywane przez dwa rejestry wskazuj¹ce na ci¹gi,
; oraz d³ugoœæ tych ci¹gów w rejestrze ecx.
; ============================================================================;
compareStringSIMD64 PROC
	; zapamiêtanie adresu Ÿród³owego do kolejnego porównania (z kolejnym elementem)
	PUSH esi
	; z rejestru eax bêdziemy korzystali do maski bitów z rejestru xmm 
	PUSH eax
	
	;pêtla porównania danego elementu
cmpSIMD:
	; sprawdzamy czy pozosta³y ci¹g do porównania jest d³u¿szy ni¿ 16 bajtów
	CMP ecx, 8
	; jeœli mniejszy, to skaczemy do tradycyjnego porównania
	JNGE cmpLoopEnter
	; porównanie 8 bajtów przy pomocy SIMD
	; za³adowanie 8B jednego ci¹gu do xmm0
	MOVQ mm0, QWORD PTR [esi]
	; za³adowanie 8B drugiego ci¹gu do xmm1
	MOVQ mm1, QWORD PTR [edi]
	; porównanie 16B
	PCMPEQB mm0, mm1
	; wyci¹gniêcie maski bitowej
	PMOVMSKB eax, mm0
	; wartoœæ 0FFh oznacza, ¿e 16B by³o równych sobie
	CMP eax, 0FFh
	; jeœli nie by³o zgodnoœci, koñczymy porównanie i wychodzimy z porównania
	JNE exitCompare
	
	; modyfikujemy odpowiednio rejestry z ci¹gami
	ADD edi, 8
	ADD esi, 8
	SUB ecx, 8
	; próbujemy dalej porównywaæ za pomoc¹ SIMD
	JMP cmpSIMD


cmpLoopEnter:
	; sprawdzenie czy jest jeszcze coœ do porównania
	CMP ecx, 0
	JE exitCompare

	; porównanie pozosta³ych bajtów zwyk³ym sposobem
cmpLoop:
	CMPSB
	LOOPE cmpLoop

exitCompare:
	; przywrócenie eax
	POP eax
	; przywrócenie pierwotnego esi dla porównania z kolejnym elementem
	POP esi  
	RET
compareStringSIMD64 ENDP

; ============================================================================;
; Procedura zwraca kod podanego ci¹gu kodowego (przekazanego przez rejestry
; esi (adres) oraz edx (d³ugoœæ)
; Zwraca w TEMP_CODEWORD_POINTER adres na dany ci¹g w s³owniku (³¹cznie z jego
; rozmiarem) oraz jego kod poprzez zmienn¹ TEMP_CODE.
; Jeœli nie ma takiego ci¹gu w s³owniku, to TEMP_CODEWORD_POINTER = 0
; ============================================================================;
dictGetCodewordId PROC PROC_DATA_PTR: DWORD
	PUSHAD

	; ustanowienie eax jako wskaŸnika na strukturê PROC_DATA_STR
	ASSUME eax : PTR PROC_DATA_STR
	MOV eax, PROC_DATA_PTR

	MOV edi, [eax].DICT_POINTER

	; aktualny numer (kod) elementu w s³owniku
	MOV ebx, 0 

	; zerujemy ecx - bêdzie s³u¿y³ za d³ugoœæ aktualnego elementu w s³owniku (jego dolne 2 bajty)
	MOV ecx, 0 
	
	; pêtla przeszukiwania kolejnych elementów s³ownika
	CLD
searchLoop:
	; zapamiêtanie adresu na wypadek znalezienia
	PUSH edi 

	; pobieramy d³ugoœæ elementu
	MOV cx, [edi] 
	ADD edi, 2

	; sprawdzamy, czy d³ugoœci s¹ równe
	CMP ecx, edx 

	; nie ma sensu porównywaæ ci¹gów o ró¿nych d³ugoœciach - przeskoczenie do kolejnego elementu
	JNE skip 

	; porównanie ci¹gów (zwrot wyniku przez ustawienie flagi
	INVOKE compareString

	; jeœli ca³y ci¹g siê zgadza
	JE equal 

skip:
	; pominiêcie nieporównanych bajtów
	; przesuniêcie edi o d³ugoœæ nieporównanych danych (lub nawet o ca³y element)
	ADD edi, ecx
next:
	; przechodzimy do kolejnego elementu
	ADD ebx, 1
	; usuwamy zapamiêtany adres ze stosu
	ADD esp, 4
	; sprawdzamy czy nie przeszukaliœmy ju¿ wszystkich elementów w s³owniku
	CMP ebx, [eax].DICT_COUNT 
	JNZ searchLoop

	; jeœli nie znaleziono elementu w s³owniku
	; zwrócenie zerowego wskaŸnika (nie znaleziono elementu)
	MOV [eax].TEMP_CODEWORD_POINTER, 0 

	JMP exit

equal:
	; na stosie zapamiêtano adres na wypadek znalezienia ci¹gu
	POP [eax].TEMP_CODEWORD_POINTER
	; ebx wskazuje na kod znalezionego ci¹gu
	MOV [eax].TEMP_CODE, ebx 

exit:
	POPAD
	; dodatkowe zwrócenie wskaŸnika przez eax
	PUSH [eax].TEMP_CODEWORD_POINTER
	POP eax
	RET
dictGetCodewordId ENDP

; ============================================================================;
; G³ówna procedura odpowiedzialna za kompresjê danych.
; Otrzymuje wskaŸnik na strukturê CompressParamsAsm
; Funkcje rejestrów:
; esi - wskaŸnik na aktualnie analizowany ci¹g Ÿród³owy
; edi - wskaŸnik na kolejn¹ pozycjê danych skompresowanych do zapisania
;       kodu kolejnego s³owa kodowego
; ecx - iloœæ bajtów pozosta³ych w danych Ÿród³owych
; edx - d³ugoœæ aktualnie analizowanego ci¹gu
; ============================================================================;
CompressThreadAsm PROC params: DWORD
	; struktura przechowuj¹ca zmienne potrzebne do wykonania kompresji
	LOCAL PROC_DATA : PROC_DATA_STR 

	; ebx dla kolejnych dwóch procedur bêdzie wskazywa³ strukturê params
	MOV ebx, params 

	; innicjalizacja zmiennych lokalnych
	MOV PROC_DATA.BLOCK_COUNT, 1
	MOV PROC_DATA.COMPRESSED_DATA_SIZE, 0
	MOV PROC_DATA.SUMMARY_COMPRESSED_DATA_SIZE, 0

	; inicjalizacja zmiennych s³ownika
	INVOKE dictInit, ADDR PROC_DATA 
	; inicjalizacja s³ownika alfabetem
	INVOKE dictInitAlphabet, ADDR PROC_DATA

	; adres danych wejœciowych
	MOV esi, [ebx + 0] 

	; rozmiar danych wejœciowych (iloœæ bajtów)
	MOV ecx, [ebx + 4] 

	; adres danych wyjœciowych (skompresowanych)
	MOV edi, [ebx + 8] 
	MOV PROC_DATA.CUR_BLOCK_POINTER, edi

	; 4 bajty rezerwujemy na rozmiar bloku
	ADD edi, 4 

	; pocz¹tkowa iloœæ wczytanych znaków - 0
	MOV edx, 0 

	; g³ówna pêtla wczytuj¹ca kolejne znaki ze Ÿród³a
dataLoop:
	; wczytaj kolejny znak
	ADD edx, 1 
	; sprawdzamy czy jest ci¹g w s³owniku - przekazanie argumentów przez esi oraz edx
	INVOKE dictGetCodewordId, ADDR PROC_DATA
	; sprawdzamy czy jest w s³owniku
	CMP eax, 0 
	; je¿eli ci¹g jest w s³owniku
	JNZ next 

	; ci¹gu nie ma w s³owniku

	; 1 wypisz kod zwi¹zany z c i zaktualizuj rozmiar danych skompresowanych
	MOV	eax, PROC_DATA.LAST_CODE
	MOV [edi], ax

	; przesuwamy siê o 2 bajty, na kolejn¹ pozycjê do zapisu kodu
	ADD edi, 2 
	ADD PROC_DATA.COMPRESSED_DATA_SIZE, 1

	; 2 dodaj przed³u¿ony ostatnio nierozpoznany ci¹g do s³ownika
	INVOKE dictAddCodeword, esi, dx, ADDR PROC_DATA

	; 3 prefiksem do kolejnego ci¹gu, jest ostatni znak z dodanego ci¹gu do s³ownika
	; dodajemy d³ugoœæ ostatniego ci¹gu
	ADD esi, edx 
	; cofamy siê o jeden znak
	SUB esi, 1 
	; ustawiamy d³ugoœæ nowego analizowanego ci¹gu na 1
	MOV edx, 1 

	; analiza czy s³ownik osi¹gn¹³ limit rozmiaru (DICT_MAX_SIZE)
	MOV eax, PROC_DATA.DICT_CUR_SIZE
	CMP PROC_DATA.DICT_MAX_SIZE, eax
	; s³ownik jeszcze nie przekroczy³ dozwolonego rozmiaru - sprawdzenie drugiego kryterium
	JG check
	; s³ownik przekroczy³ dozwolony rozmiar - tworzymy nowy
	JMP newDict

check:
	; analiza czy s³ownik osi¹gn¹³ limit elementów (na sztywno ustawiony na 16384 elementów)
	CMP PROC_DATA.DICT_COUNT, 16384
	; nie osi¹gn¹³ limitu elementów
	JNGE ok

newDict:
	; s³ownik osi¹gn¹³ limit, trzeba wyczyœciæ, utworzyæ nowy, i rozpocz¹æ nowy blok
	; 1. inicjalizacja nowego s³ownika
	; ebx dla kolejnych dwóch procedur bêdzie wskazywa³ strukturê params
	MOV ebx, params 
	; inicjalizacja zmiennych s³ownika
	INVOKE dictInit, ADDR PROC_DATA 
	; inicjalizacja s³ownika alfabetem
	INVOKE dictInitAlphabet, ADDR PROC_DATA

	; 2. zapisanie rozmiaru bloku, dodanie do sumarycznego rozmiaru
	; pobranie adresu pocz¹tku bloku
	MOV ebx, PROC_DATA.CUR_BLOCK_POINTER 
	MOV eax, PROC_DATA.COMPRESSED_DATA_SIZE
	MOV [ebx], eax
	ADD PROC_DATA.SUMMARY_COMPRESSED_DATA_SIZE, eax

	; 3. zwiêkszenie numeracji iloœci bloków
	ADD PROC_DATA.BLOCK_COUNT, 1
	
	; 4. ustawienie nowego bloku
	; otrzymanie iloœci bajtów danych skompresowanych
	ADD eax, eax 
	; dodanie 4 bajtów rozmiaru z pocz¹tku bloku
	ADD eax, 4 
	; ustalenie nowego pocz¹tku bloku
	ADD PROC_DATA.CUR_BLOCK_POINTER, eax 
	; nowe miejsce zapisu kodów
	MOV edi, PROC_DATA.CUR_BLOCK_POINTER 
	; zarezerwowanie 4 bajtów na rozmiar nowego bloku
	ADD edi, 4 
	; wyzerowanie aktualnego rozmiaru danych skompresowanych
	MOV PROC_DATA.COMPRESSED_DATA_SIZE, 0 

ok:
	; aktualizujemy ostatnio rozpoznany ci¹g
	; musi byæ w s³owniku, poniewa¿ jest to pojedynczy znak alfabetu
	INVOKE dictGetCodewordId, ADDR PROC_DATA

next:
	MOV eax, PROC_DATA.TEMP_CODE
	; zapamiêtaj ostatnio rozpoznany kod
	MOV PROC_DATA.LAST_CODE, eax 

	DEC ecx
	JNZ dataLoop

	; na koñcu wypisujemy kod zwi¹zany z ostatnio rozpoznanym symbolem i aktualizujemy rozmiar danych skompresowanych
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