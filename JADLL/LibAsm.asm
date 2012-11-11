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
	PUSHAD ; zachowanie rejestrów

	MOV ecx, 0
	MOV cx, len
	ADD DICT_CUR_SIZE, ecx ; zwiêkszenie rozmiaru s³ownika
	ADD DICT_COUNT, 1 ; zwiêkszenie liczby elementów w s³owniku

	MOV esi, codeword ; ustalamy Ÿród³owy adres odczytu danych
	MOV edi, DICT_END_POINTER ; ustawiamy docelowy adres zapisu danych
	MOV [edi], cx ; zapisanie d³ugoœci s³owa kodowego
	ADD edi, 2 ; ustawienie siê na miejsce zapisu s³owa kodowego
	CLD
	REP MOVSB ; skopiowanie s³owa kodowego do s³ownika

	MOV DICT_END_POINTER, edi ; ustalenie nowego koñca s³ownika

	POPAD ; odtworzenie rejestrów
	RET
dictAddCodeword ENDP

; ============================================================================;
; Procedura inicjuje s³ownik alfabetem.
; ============================================================================;
dictInitAlphabet PROC
	; zachowanie rejestrów
	PUSHAD
	MOV ecx, [ebx + 32] ; odczytanie iloœci znaków w alfabecie
	MOV esi, [ebx + 28] ; wskaŸnik na alfabet

alphabetLoop:
	INVOKE dictAddCodeword, esi, 1 ; dodaj pojedynczy znak alfabetu do s³ownika
	ADD esi, 1
	LOOP alphabetLoop

	POPAD
	RET
dictInitAlphabet ENDP

; ============================================================================;
; Procedura inicjuj¹ca zmienne DICT_POINTER oraz DICT_END_POINTER
; ============================================================================;
dictInit PROC
	MOV eax, [ebx + 20] ; ustawiamy wskaŸnik na miejsce dla s³ownika
	MOV DICT_POINTER, eax
	MOV DICT_END_POINTER, eax ; pierwszym wolnym bajtem jest pocz¹tek
	MOV eax, [ebx + 24] ; ustawiamy maksymalny rozmiar danych w s³owniku
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
	MOV ebx, 0 ; aktualny numer (kod) elementu w s³owniku
	MOV ecx, 0 ; zerujemy ecx - bêdzie s³u¿y³ za d³ugoœæ aktualnego elementu w s³owniku (jego dolne 2 bajty)
	
	; pêtla przeszukiwania kolejnych elementów s³ownika
	CLD
searchLoop:
	MOV eax, edi ; zapamiêtanie adresu na wypadek znalezienia
	MOV cx, [edi] ; pobieramy d³ugoœæ elementu
	ADD edi, 2
	CMP ecx, edx ; sprawdzamy, czy d³ugoœci s¹ równe
	JNE skip ; nie ma sensu porównywaæ ci¹gów o ró¿nych d³ugoœciach - przeskoczenie do kolejnego elementu

	;pêtla porównania danego elementu
	PUSH esi ; zapamiêtanie adresu Ÿród³owego do kolejnego porównania (z kolejnym elementem)
cmpLoop:
	CMPSB
	LOOPE cmpLoop
	POP esi ; przywrócenie pierwotnego esi dla porównania z kolejnym elementem
	JE equal ; jeœli ca³y ci¹g siê zgadza

skip: ; pominiêcie nieporównanych bajtów
	ADD edi, ecx ; przesuniêcie edi o d³ugoœæ nieporównanych danych (lub nawet o ca³y element)
next:
	ADD ebx, 1 ; przechodzimy do kolejnego elementu
	CMP ebx, DICT_COUNT ; sprawdzamy czy nie przeszukaliœmy ju¿ wszystkich elementów w s³owniku
	JNZ searchLoop

	; jeœli nie znaleziono elementu w s³owniku
	MOV TEMP_CODEWORD_POINTER, 0 ; zwrócenie zerowego wskaŸnika (nie znaleziono elementu)
	JMP exit

equal:
	MOV TEMP_CODEWORD_POINTER, eax ; w eax zapamiêtano adres na wypadek znalezienia ci¹gu
	MOV TEMP_CODE, ebx ; ebx wskazuje na kod znalezionego ci¹gu

exit:
	POPAD
	MOV eax, TEMP_CODEWORD_POINTER ; dodatkowe zwrócenie wskaŸnika przez eax
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
	MOV ebx, params ; ebx dla kolejnych dwóch procedur bêdzie wskazywa³ strukturê params
	INVOKE dictInit ; inicjalizacja zmiennych s³ownika
	INVOKE dictInitAlphabet ; inicjalizacja s³ownika alfabetem

	MOV esi, [ebx + 0] ; adres danych wejœciowych
	MOV ecx, [ebx + 4] ; rozmiar danych wejœciowych (iloœæ bajtów)
	MOV edi, [ebx + 8] ; adres danych wyjœciowych (skompresowanych)
	MOV CUR_BLOCK_POINTER, edi
	ADD edi, 4 ; 4 bajty rezerwujemy na rozmiar bloku
	MOV edx, 0 ; pocz¹tkowa iloœæ wczytanych znaków - 0

	; g³ówna pêtla wczytuj¹ca kolejne znaki ze Ÿród³a
dataLoop:
	ADD edx, 1 ; wczytaj kolejny znak
	INVOKE dictGetCodewordId ; sprawdzamy czy jest ci¹g w s³owniku - przekazanie argumentów przez esi oraz edx
	CMP eax, 0 ; sprawdzamy czy jest w s³owniku
	JNZ next ; je¿eli ci¹g jest w s³owniku

	; ci¹gu nie ma w s³owniku

	; 1 wypisz kod zwi¹zany z c i zaktualizuj rozmiar danych skompresowanych
	MOV	eax, LAST_CODE
	MOV [edi], ax
	ADD edi, 2 ; przesuwamy siê o 2 bajty, na kolejn¹ pozycjê do zapisu kodu
	ADD COMPRESSED_DATA_SIZE, 1

	; 2 dodaj przed³u¿ony ostatnio nierozpoznany ci¹g do s³ownika
	INVOKE dictAddCodeword, esi, dx

	; 3 prefiksem do kolejnego ci¹gu, jest ostatni znak z dodanego ci¹gu do s³ownika
	ADD esi, edx ; dodajemy d³ugoœæ ostatniego ci¹gu
	SUB esi, 1 ; cofamy siê o jeden znak
	MOV edx, 1 ; ustawiamy d³ugoœæ nowego analizowanego ci¹gu na 1

	; analiza czy s³ownik osi¹gn¹³ limit rozmiaru (DICT_MAX_SIZE)
	MOV eax, DICT_CUR_SIZE
	CMP DICT_MAX_SIZE, eax
	JG ok ; rozmiar jeszcze nie przekroczy³

	; analiza czy s³ownik osi¹gn¹³ limit elementów (na sztywno ustawiony na 65536 elementów)
	CMP DICT_COUNT, 65536
	JG ok ; nie osi¹gn¹³ limitu elementów

	; s³ownik osi¹gn¹³ limit, trzeba wyczyœciæ, utworzyæ nowy, i rozpocz¹æ nowy blok
	; 1. inicjalizacja nowego s³ownika
	MOV ebx, params ; ebx dla kolejnych dwóch procedur bêdzie wskazywa³ strukturê params
	INVOKE dictInit ; inicjalizacja zmiennych s³ownika
	INVOKE dictInitAlphabet ; inicjalizacja s³ownika alfabetem

	; 2. zapisanie rozmiaru bloku, dodanie do sumarycznego rozmiaru
	MOV ebx, CUR_BLOCK_POINTER ; pobranie adresu pocz¹tku bloku
	MOV eax, COMPRESSED_DATA_SIZE
	MOV [ebx], eax
	ADD SUMMARY_COMPRESSED_DATA_SIZE, eax

	; 3. zwiêkszenie numeracji iloœci bloków
	ADD BLOCK_COUNT, 1
	
	; 4. ustawienie nowego bloku
	ADD eax, eax ; otrzymanie iloœci bajtów danych skompresowanych
	ADD eax, 4 ; dodanie 4 bajtów rozmiaru z pocz¹tku bloku
	ADD CUR_BLOCK_POINTER, eax ; ustalenie nowego pocz¹tku bloku
	MOV edi, CUR_BLOCK_POINTER ; nowe miejsce zapisu kodów
	ADD edi, 4 ; zarezerwowanie 4 bajtów na rozmiar nowego bloku
	MOV COMPRESSED_DATA_SIZE, 0 ; wyzerowanie aktualnego rozmiaru danych skompresowanych

ok:
	; aktualizujemy ostatnio rozpoznany ci¹g
	INVOKE dictGetCodewordId ; musi byæ w s³owniku, poniewa¿ jest to pojedynczy znak alfabetu
	;MOV LAST_CODEWORD_POINTER, TEMP_CODEWORD_POINTER

next:
	;MOV LAST_CODEWORD_POINTER, TEMP_CODEWORD_POINTER ; zapamiêtaj ostatnio rozpoznany ci¹g
	MOV eax, TEMP_CODE
	MOV LAST_CODE, eax ; zapamiêtaj ostatnio rozpoznany kod

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