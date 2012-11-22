.686
.model flat, stdcall

;EXTERN printf:proc ; mo�liwo�� skorzystania z wypisywania informacji na ekran
printf PROTO C :VARARG

.data
;ALPHABET_POINTER DD 0 ; wska�nik na alfabet
;ALPHABET_SIZE DB 0 ;ilo�� s��w w alfabecie

DICT_POINTER DD 0 ; wska�nik na obszar pami�ci dla s�ownika
DICT_END_POINTER DD 0 ; wska�nik na pierwszy wolny bajt s�ownika 
DICT_MAX_SIZE DD 0 ; maksymalny rozmiar s�ownika
DICT_CUR_SIZE DD 0 ; aktualny rozmiar s�ownika
DICT_COUNT DD 0 ; ilo�� element�w w s�owniku

BLOCK_COUNT DD 1 ; ilo�� blok�w
CUR_BLOCK_POINTER DD 0 ; wska�nik na aktualnie zapisywany blok
;CUR_DATA_POINTER DD 0 ; wska�nik na aktualnie przetwarzany ci�g
;CUR_DATA_LEN DD 0 ; d�ugo�� analizowanego ci�gu

COMPRESSED_DATA_SIZE DD 0 ; ilo�� skompresowanych danych w bloku (kod�w)
SUMMARY_COMPRESSED_DATA_SIZE DD 0 ; og�lnie ilo�� skompresowanych danych (kod�w)

;LAST_CODEWORD_POINTER DD 0 ; ostatnio rozpoznany ci�g zaczynaj�c od dw�ch bajt�w orke�laj�cych jego d�ugo��)
TEMP_CODEWORD_POINTER DD 0 ; tymczasowo rozpoznany ci�g (zaczynaj�c od dw�ch bajt�w okre�laj�cych jego d�ugo��)
LAST_CODE DD 0 ; kod ostatnio rozpoznanego ci�gu
TEMP_CODE DD 0 ; tymczasowy kod rozpozanego ci�gu

.code

; ============================================================================;
; Procedura dodaje nowe s�owo kodowe do s�ownika.
; Przyjmowane argumenty:
; codeword - wska�nik na s�owo kodowe
; len - d�ugo�� s�owa kodowego
; ============================================================================;
dictAddCodeword PROC codeword: DWORD, len: WORD
	; zachowanie rejestr�w
	PUSHAD

	; ustawienie licznika (d�ugo�� s�owa kodowego)
	MOV ecx, 0
	MOV cx, len

	; zwi�kszenie rozmiaru s�ownika
	ADD DICT_CUR_SIZE, ecx

	; zwi�kszenie liczby element�w w s�owniku
	ADD DICT_COUNT, 1

	; ustalamy �r�d�owy adres odczytu danych
	MOV esi, codeword

	; ustawiamy docelowy adres zapisu danych
	MOV edi, DICT_END_POINTER 

	; zapisanie d�ugo�ci s�owa kodowego
	MOV [edi], cx 

	; ustawienie si� na miejsce zapisu s�owa kodowego
	ADD edi, 2 
	CLD
	; skopiowanie s�owa kodowego do s�ownika
	REP MOVSB 

	; ustalenie nowego ko�ca s�ownika
	MOV DICT_END_POINTER, edi 

	; odtworzenie rejestr�w
	POPAD
	RET
dictAddCodeword ENDP

; ============================================================================;
; Procedura inicjuje s�ownik alfabetem.
; ============================================================================;
dictInitAlphabet PROC
	; zachowanie rejestr�w
	PUSHAD
	; odczytanie ilo�ci znak�w w alfabecie
	MOV ecx, [ebx + 32] 
	; wska�nik na alfabet
	MOV esi, [ebx + 28] 

alphabetLoop:
	; dodaj pojedynczy znak alfabetu do s�ownika
	INVOKE dictAddCodeword, esi, 1 
	ADD esi, 1
	LOOP alphabetLoop

	POPAD
	RET
dictInitAlphabet ENDP

; ============================================================================;
; Procedura inicjuj�ca zmienne DICT_POINTER oraz DICT_END_POINTER
; ============================================================================;
dictInit PROC
	; ustawiamy wska�nik na miejsce dla s�ownika
	MOV eax, [ebx + 20] 
	MOV DICT_POINTER, eax

	; pierwszym wolnym bajtem jest pocz�tek
	MOV DICT_END_POINTER, eax 

	; ustawiamy maksymalny rozmiar danych w s�owniku
	MOV eax, [ebx + 24] 
	MOV DICT_MAX_SIZE, eax
	MOV DICT_CUR_SIZE, 0
	MOV DICT_COUNT, 0
	RET
dictInit ENDP

; ============================================================================;
; Procedura zwraca kod podanego ci�gu kodowego (przekazanego przez rejestry
; esi (adres) oraz edx (d�ugo��)
; Zwraca w TEMP_CODEWORD_POINTER adres na dany ci�g w s�owniku (��cznie z jego
; rozmiarem) oraz jego kod poprzez zmienn� TEMP_CODE.
; Je�li nie ma takiego ci�gu w s�owniku, to TEMP_CODEWORD_POINTER = 0
; ============================================================================;
dictGetCodewordId PROC
	PUSHAD

	MOV edi, DICT_POINTER

	; aktualny numer (kod) elementu w s�owniku
	MOV ebx, 0 

	; zerujemy ecx - b�dzie s�u�y� za d�ugo�� aktualnego elementu w s�owniku (jego dolne 2 bajty)
	MOV ecx, 0 
	
	; p�tla przeszukiwania kolejnych element�w s�ownika
	CLD
searchLoop:
	; zapami�tanie adresu na wypadek znalezienia
	MOV eax, edi 

	; pobieramy d�ugo�� elementu
	MOV cx, [edi] 
	ADD edi, 2

	; sprawdzamy, czy d�ugo�ci s� r�wne
	CMP ecx, edx 

	; nie ma sensu por�wnywa� ci�g�w o r�nych d�ugo�ciach - przeskoczenie do kolejnego elementu
	JNE skip 

	; zapami�tanie adresu �r�d�owego do kolejnego por�wnania (z kolejnym elementem)
	PUSH esi 

	;p�tla por�wnania danego elementu
cmpLoop:
	CMPSB
	LOOPE cmpLoop
	; przywr�cenie pierwotnego esi dla por�wnania z kolejnym elementem
	POP esi 
	; je�li ca�y ci�g si� zgadza
	JE equal 

skip:
	; pomini�cie niepor�wnanych bajt�w
	; przesuni�cie edi o d�ugo�� niepor�wnanych danych (lub nawet o ca�y element)
	ADD edi, ecx
next:
	; przechodzimy do kolejnego elementu
	ADD ebx, 1
	; sprawdzamy czy nie przeszukali�my ju� wszystkich element�w w s�owniku
	CMP ebx, DICT_COUNT 
	JNZ searchLoop

	; je�li nie znaleziono elementu w s�owniku
	; zwr�cenie zerowego wska�nika (nie znaleziono elementu)
	MOV TEMP_CODEWORD_POINTER, 0 
	JMP exit

equal:
	; w eax zapami�tano adres na wypadek znalezienia ci�gu
	MOV TEMP_CODEWORD_POINTER, eax
	; ebx wskazuje na kod znalezionego ci�gu
	MOV TEMP_CODE, ebx 

exit:
	POPAD
	; dodatkowe zwr�cenie wska�nika przez eax
	MOV eax, TEMP_CODEWORD_POINTER 
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
	; ebx dla kolejnych dw�ch procedur b�dzie wskazywa� struktur� params
	MOV ebx, params 

	; inicjalizacja zmiennych s�ownika
	INVOKE dictInit 
	; inicjalizacja s�ownika alfabetem
	INVOKE dictInitAlphabet 

	; adres danych wej�ciowych
	MOV esi, [ebx + 0] 

	; rozmiar danych wej�ciowych (ilo�� bajt�w)
	MOV ecx, [ebx + 4] 

	; adres danych wyj�ciowych (skompresowanych)
	MOV edi, [ebx + 8] 
	MOV CUR_BLOCK_POINTER, edi

	; 4 bajty rezerwujemy na rozmiar bloku
	ADD edi, 4 

	; pocz�tkowa ilo�� wczytanych znak�w - 0
	MOV edx, 0 

	; g��wna p�tla wczytuj�ca kolejne znaki ze �r�d�a
dataLoop:
	; wczytaj kolejny znak
	ADD edx, 1 
	; sprawdzamy czy jest ci�g w s�owniku - przekazanie argument�w przez esi oraz edx
	INVOKE dictGetCodewordId 
	; sprawdzamy czy jest w s�owniku
	CMP eax, 0 
	; je�eli ci�g jest w s�owniku
	JNZ next 

	; ci�gu nie ma w s�owniku

	; 1 wypisz kod zwi�zany z c i zaktualizuj rozmiar danych skompresowanych
	MOV	eax, LAST_CODE
	MOV [edi], ax

	; przesuwamy si� o 2 bajty, na kolejn� pozycj� do zapisu kodu
	ADD edi, 2 
	ADD COMPRESSED_DATA_SIZE, 1

	; 2 dodaj przed�u�ony ostatnio nierozpoznany ci�g do s�ownika
	INVOKE dictAddCodeword, esi, dx

	; 3 prefiksem do kolejnego ci�gu, jest ostatni znak z dodanego ci�gu do s�ownika
	; dodajemy d�ugo�� ostatniego ci�gu
	ADD esi, edx 
	; cofamy si� o jeden znak
	SUB esi, 1 
	; ustawiamy d�ugo�� nowego analizowanego ci�gu na 1
	MOV edx, 1 

	; analiza czy s�ownik osi�gn�� limit rozmiaru (DICT_MAX_SIZE)
	MOV eax, DICT_CUR_SIZE
	CMP DICT_MAX_SIZE, eax
	; rozmiar jeszcze nie przekroczy�
	JG ok 

	; analiza czy s�ownik osi�gn�� limit element�w (na sztywno ustawiony na 65536 element�w)
	CMP DICT_COUNT, 65536
	; nie osi�gn�� limitu element�w
	JG ok 

	; s�ownik osi�gn�� limit, trzeba wyczy�ci�, utworzy� nowy, i rozpocz�� nowy blok
	; 1. inicjalizacja nowego s�ownika
	; ebx dla kolejnych dw�ch procedur b�dzie wskazywa� struktur� params
	MOV ebx, params 
	; inicjalizacja zmiennych s�ownika
	INVOKE dictInit 
	; inicjalizacja s�ownika alfabetem
	INVOKE dictInitAlphabet 

	; 2. zapisanie rozmiaru bloku, dodanie do sumarycznego rozmiaru
	; pobranie adresu pocz�tku bloku
	MOV ebx, CUR_BLOCK_POINTER 
	MOV eax, COMPRESSED_DATA_SIZE
	MOV [ebx], eax
	ADD SUMMARY_COMPRESSED_DATA_SIZE, eax

	; 3. zwi�kszenie numeracji ilo�ci blok�w
	ADD BLOCK_COUNT, 1
	
	; 4. ustawienie nowego bloku
	; otrzymanie ilo�ci bajt�w danych skompresowanych
	ADD eax, eax 
	; dodanie 4 bajt�w rozmiaru z pocz�tku bloku
	ADD eax, 4 
	; ustalenie nowego pocz�tku bloku
	ADD CUR_BLOCK_POINTER, eax 
	; nowe miejsce zapisu kod�w
	MOV edi, CUR_BLOCK_POINTER 
	; zarezerwowanie 4 bajt�w na rozmiar nowego bloku
	ADD edi, 4 
	; wyzerowanie aktualnego rozmiaru danych skompresowanych
	MOV COMPRESSED_DATA_SIZE, 0 

ok:
	; aktualizujemy ostatnio rozpoznany ci�g
	; musi by� w s�owniku, poniewa� jest to pojedynczy znak alfabetu
	INVOKE dictGetCodewordId 
	;MOV LAST_CODEWORD_POINTER, TEMP_CODEWORD_POINTER

next:
	;MOV LAST_CODEWORD_POINTER, TEMP_CODEWORD_POINTER ; zapami�taj ostatnio rozpoznany ci�g
	MOV eax, TEMP_CODE
	; zapami�taj ostatnio rozpoznany kod
	MOV LAST_CODE, eax 

	DEC ecx
	JNZ dataLoop

	; na ko�cu wypisujemy kod zwi�zany z ostatnio rozpoznanym symbolem i aktualizujemy rozmiar danych skompresowanych
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
	MOV ebx, var ; ebx dla kolejnych dw�ch procedur b�dzie wskazywa� struktur� params
	INVOKE dictInit ; inicjalizacja zmiennych s�ownika
	INVOKE dictInitAlphabet ; inicjalizacja s�ownika alfabetem
	RET
TestProc endp


; ============================================================================;
; 
; ============================================================================;

; ============================================================================;
; 
; ============================================================================;


end