.686
.model flat, stdcall

;EXTERN printf:proc ; mo¿liwoœæ skorzystania z wypisywania informacji na ekran
printf PROTO C :VARARG

.data
;ALPHABET_POINTER DD 0 ; wskaŸnik na alfabet
;ALPHABET_SIZE DB 0 ;iloœæ s³ów w alfabecie

DICT_POINTER DD 0 ; wskaŸnik na obszar pamiêci dla s³ownika
DICT_END_POINTER DD 0 ; wskaŸnik na pierwszy wolny bajt s³ownika 
DICT_MAX_SIZE DD 0 ; maksymalny rozmiar s³ownika
DICT_CUR_SIZE DD 0 ; aktualny rozmiar s³ownika
DICT_COUNT DD 0 ; iloœæ elementów w s³owniku

BLOCK_COUNT DD 1 ; iloœæ bloków
CUR_BLOCK_POINTER DD 0 ; wskaŸnik na aktualnie zapisywany blok
;CUR_DATA_POINTER DD 0 ; wskaŸnik na aktualnie przetwarzany ci¹g
;CUR_DATA_LEN DD 0 ; d³ugoœæ analizowanego ci¹gu

COMPRESSED_DATA_SIZE DD 0 ; iloœæ skompresowanych danych w bloku (kodów)
SUMMARY_COMPRESSED_DATA_SIZE DD 0 ; ogólnie iloœæ skompresowanych danych (kodów)

;LAST_CODEWORD_POINTER DD 0 ; ostatnio rozpoznany ci¹g zaczynaj¹c od dwóch bajtów orkeœlaj¹cych jego d³ugoœæ)
TEMP_CODEWORD_POINTER DD 0 ; tymczasowo rozpoznany ci¹g (zaczynaj¹c od dwóch bajtów okreœlaj¹cych jego d³ugoœæ)
LAST_CODE DD 0 ; kod ostatnio rozpoznanego ci¹gu
TEMP_CODE DD 0 ; tymczasowy kod rozpozanego ci¹gu

.code

; ============================================================================;
; Procedura dodaje nowe s³owo kodowe do s³ownika.
; Przyjmowane argumenty:
; codeword - wskaŸnik na s³owo kodowe
; len - d³ugoœæ s³owa kodowego
; ============================================================================;
dictAddCodeword PROC codeword: DWORD, len: WORD
	; zachowanie rejestrów
	PUSHAD

	; ustawienie licznika (d³ugoœæ s³owa kodowego)
	MOV ecx, 0
	MOV cx, len

	; zwiêkszenie rozmiaru s³ownika
	ADD DICT_CUR_SIZE, ecx

	; zwiêkszenie liczby elementów w s³owniku
	ADD DICT_COUNT, 1

	; ustalamy Ÿród³owy adres odczytu danych
	MOV esi, codeword

	; ustawiamy docelowy adres zapisu danych
	MOV edi, DICT_END_POINTER 

	; zapisanie d³ugoœci s³owa kodowego
	MOV [edi], cx 

	; ustawienie siê na miejsce zapisu s³owa kodowego
	ADD edi, 2 
	CLD
	; skopiowanie s³owa kodowego do s³ownika
	REP MOVSB 

	; ustalenie nowego koñca s³ownika
	MOV DICT_END_POINTER, edi 

	; odtworzenie rejestrów
	POPAD
	RET
dictAddCodeword ENDP

; ============================================================================;
; Procedura inicjuje s³ownik alfabetem.
; ============================================================================;
dictInitAlphabet PROC
	; zachowanie rejestrów
	PUSHAD
	; odczytanie iloœci znaków w alfabecie
	MOV ecx, [ebx + 32] 
	; wskaŸnik na alfabet
	MOV esi, [ebx + 28] 

alphabetLoop:
	; dodaj pojedynczy znak alfabetu do s³ownika
	INVOKE dictAddCodeword, esi, 1 
	ADD esi, 1
	LOOP alphabetLoop

	POPAD
	RET
dictInitAlphabet ENDP

; ============================================================================;
; Procedura inicjuj¹ca zmienne DICT_POINTER oraz DICT_END_POINTER
; ============================================================================;
dictInit PROC
	; ustawiamy wskaŸnik na miejsce dla s³ownika
	MOV eax, [ebx + 20] 
	MOV DICT_POINTER, eax

	; pierwszym wolnym bajtem jest pocz¹tek
	MOV DICT_END_POINTER, eax 

	; ustawiamy maksymalny rozmiar danych w s³owniku
	MOV eax, [ebx + 24] 
	MOV DICT_MAX_SIZE, eax
	MOV DICT_CUR_SIZE, 0
	MOV DICT_COUNT, 0
	RET
dictInit ENDP

; ============================================================================;
; Procedura zwraca kod podanego ci¹gu kodowego (przekazanego przez rejestry
; esi (adres) oraz edx (d³ugoœæ)
; Zwraca w TEMP_CODEWORD_POINTER adres na dany ci¹g w s³owniku (³¹cznie z jego
; rozmiarem) oraz jego kod poprzez zmienn¹ TEMP_CODE.
; Jeœli nie ma takiego ci¹gu w s³owniku, to TEMP_CODEWORD_POINTER = 0
; ============================================================================;
dictGetCodewordId PROC
	PUSHAD

	MOV edi, DICT_POINTER

	; aktualny numer (kod) elementu w s³owniku
	MOV ebx, 0 

	; zerujemy ecx - bêdzie s³u¿y³ za d³ugoœæ aktualnego elementu w s³owniku (jego dolne 2 bajty)
	MOV ecx, 0 
	
	; pêtla przeszukiwania kolejnych elementów s³ownika
	CLD
searchLoop:
	; zapamiêtanie adresu na wypadek znalezienia
	MOV eax, edi 

	; pobieramy d³ugoœæ elementu
	MOV cx, [edi] 
	ADD edi, 2

	; sprawdzamy, czy d³ugoœci s¹ równe
	CMP ecx, edx 

	; nie ma sensu porównywaæ ci¹gów o ró¿nych d³ugoœciach - przeskoczenie do kolejnego elementu
	JNE skip 

	; zapamiêtanie adresu Ÿród³owego do kolejnego porównania (z kolejnym elementem)
	PUSH esi 

	;pêtla porównania danego elementu
cmpLoop:
	CMPSB
	LOOPE cmpLoop
	; przywrócenie pierwotnego esi dla porównania z kolejnym elementem
	POP esi 
	; jeœli ca³y ci¹g siê zgadza
	JE equal 

skip:
	; pominiêcie nieporównanych bajtów
	; przesuniêcie edi o d³ugoœæ nieporównanych danych (lub nawet o ca³y element)
	ADD edi, ecx
next:
	; przechodzimy do kolejnego elementu
	ADD ebx, 1
	; sprawdzamy czy nie przeszukaliœmy ju¿ wszystkich elementów w s³owniku
	CMP ebx, DICT_COUNT 
	JNZ searchLoop

	; jeœli nie znaleziono elementu w s³owniku
	; zwrócenie zerowego wskaŸnika (nie znaleziono elementu)
	MOV TEMP_CODEWORD_POINTER, 0 
	JMP exit

equal:
	; w eax zapamiêtano adres na wypadek znalezienia ci¹gu
	MOV TEMP_CODEWORD_POINTER, eax
	; ebx wskazuje na kod znalezionego ci¹gu
	MOV TEMP_CODE, ebx 

exit:
	POPAD
	; dodatkowe zwrócenie wskaŸnika przez eax
	MOV eax, TEMP_CODEWORD_POINTER 
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
	; ebx dla kolejnych dwóch procedur bêdzie wskazywa³ strukturê params
	MOV ebx, params 

	; inicjalizacja zmiennych s³ownika
	INVOKE dictInit 
	; inicjalizacja s³ownika alfabetem
	INVOKE dictInitAlphabet 

	; adres danych wejœciowych
	MOV esi, [ebx + 0] 

	; rozmiar danych wejœciowych (iloœæ bajtów)
	MOV ecx, [ebx + 4] 

	; adres danych wyjœciowych (skompresowanych)
	MOV edi, [ebx + 8] 
	MOV CUR_BLOCK_POINTER, edi

	; 4 bajty rezerwujemy na rozmiar bloku
	ADD edi, 4 

	; pocz¹tkowa iloœæ wczytanych znaków - 0
	MOV edx, 0 

	; g³ówna pêtla wczytuj¹ca kolejne znaki ze Ÿród³a
dataLoop:
	; wczytaj kolejny znak
	ADD edx, 1 
	; sprawdzamy czy jest ci¹g w s³owniku - przekazanie argumentów przez esi oraz edx
	INVOKE dictGetCodewordId 
	; sprawdzamy czy jest w s³owniku
	CMP eax, 0 
	; je¿eli ci¹g jest w s³owniku
	JNZ next 

	; ci¹gu nie ma w s³owniku

	; 1 wypisz kod zwi¹zany z c i zaktualizuj rozmiar danych skompresowanych
	MOV	eax, LAST_CODE
	MOV [edi], ax

	; przesuwamy siê o 2 bajty, na kolejn¹ pozycjê do zapisu kodu
	ADD edi, 2 
	ADD COMPRESSED_DATA_SIZE, 1

	; 2 dodaj przed³u¿ony ostatnio nierozpoznany ci¹g do s³ownika
	INVOKE dictAddCodeword, esi, dx

	; 3 prefiksem do kolejnego ci¹gu, jest ostatni znak z dodanego ci¹gu do s³ownika
	; dodajemy d³ugoœæ ostatniego ci¹gu
	ADD esi, edx 
	; cofamy siê o jeden znak
	SUB esi, 1 
	; ustawiamy d³ugoœæ nowego analizowanego ci¹gu na 1
	MOV edx, 1 

	; analiza czy s³ownik osi¹gn¹³ limit rozmiaru (DICT_MAX_SIZE)
	MOV eax, DICT_CUR_SIZE
	CMP DICT_MAX_SIZE, eax
	; rozmiar jeszcze nie przekroczy³
	JG ok 

	; analiza czy s³ownik osi¹gn¹³ limit elementów (na sztywno ustawiony na 65536 elementów)
	CMP DICT_COUNT, 65536
	; nie osi¹gn¹³ limitu elementów
	JG ok 

	; s³ownik osi¹gn¹³ limit, trzeba wyczyœciæ, utworzyæ nowy, i rozpocz¹æ nowy blok
	; 1. inicjalizacja nowego s³ownika
	; ebx dla kolejnych dwóch procedur bêdzie wskazywa³ strukturê params
	MOV ebx, params 
	; inicjalizacja zmiennych s³ownika
	INVOKE dictInit 
	; inicjalizacja s³ownika alfabetem
	INVOKE dictInitAlphabet 

	; 2. zapisanie rozmiaru bloku, dodanie do sumarycznego rozmiaru
	; pobranie adresu pocz¹tku bloku
	MOV ebx, CUR_BLOCK_POINTER 
	MOV eax, COMPRESSED_DATA_SIZE
	MOV [ebx], eax
	ADD SUMMARY_COMPRESSED_DATA_SIZE, eax

	; 3. zwiêkszenie numeracji iloœci bloków
	ADD BLOCK_COUNT, 1
	
	; 4. ustawienie nowego bloku
	; otrzymanie iloœci bajtów danych skompresowanych
	ADD eax, eax 
	; dodanie 4 bajtów rozmiaru z pocz¹tku bloku
	ADD eax, 4 
	; ustalenie nowego pocz¹tku bloku
	ADD CUR_BLOCK_POINTER, eax 
	; nowe miejsce zapisu kodów
	MOV edi, CUR_BLOCK_POINTER 
	; zarezerwowanie 4 bajtów na rozmiar nowego bloku
	ADD edi, 4 
	; wyzerowanie aktualnego rozmiaru danych skompresowanych
	MOV COMPRESSED_DATA_SIZE, 0 

ok:
	; aktualizujemy ostatnio rozpoznany ci¹g
	; musi byæ w s³owniku, poniewa¿ jest to pojedynczy znak alfabetu
	INVOKE dictGetCodewordId 
	;MOV LAST_CODEWORD_POINTER, TEMP_CODEWORD_POINTER

next:
	;MOV LAST_CODEWORD_POINTER, TEMP_CODEWORD_POINTER ; zapamiêtaj ostatnio rozpoznany ci¹g
	MOV eax, TEMP_CODE
	; zapamiêtaj ostatnio rozpoznany kod
	MOV LAST_CODE, eax 

	DEC ecx
	JNZ dataLoop

	; na koñcu wypisujemy kod zwi¹zany z ostatnio rozpoznanym symbolem i aktualizujemy rozmiar danych skompresowanych
	MOV	eax, LAST_CODE
	MOV [edi], ax
	ADD COMPRESSED_DATA_SIZE, 1
	MOV eax, COMPRESSED_DATA_SIZE
	ADD SUMMARY_COMPRESSED_DATA_SIZE, eax

	; zapisujemy rozmiar ostatniego bloku
	MOV eax, COMPRESSED_DATA_SIZE
	MOV edi, CUR_BLOCK_POINTER
	MOV [edi], eax

	; aktualizujemy dane zwrotne w parametrach kompresji
	MOV ebx, params
	MOV eax, SUMMARY_COMPRESSED_DATA_SIZE
	MOV [ebx + 12], eax

	MOV eax, BLOCK_COUNT
	MOV [ebx + 16], eax

	RET
CompressThreadAsm ENDP


; ============================================================================;
TestProc proc var: DWORD
	MOV ebx, var ; ebx dla kolejnych dwóch procedur bêdzie wskazywa³ strukturê params
	INVOKE dictInit ; inicjalizacja zmiennych s³ownika
	INVOKE dictInitAlphabet ; inicjalizacja s³ownika alfabetem
	RET
TestProc endp


; ============================================================================;
; 
; ============================================================================;

; ============================================================================;
; 
; ============================================================================;


end