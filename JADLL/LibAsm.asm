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
	PUSHAD ; zachowanie rejestr�w

	MOV ecx, 0
	MOV cx, len
	ADD DICT_CUR_SIZE, ecx ; zwi�kszenie rozmiaru s�ownika
	ADD DICT_COUNT, 1 ; zwi�kszenie liczby element�w w s�owniku

	MOV esi, codeword ; ustalamy �r�d�owy adres odczytu danych
	MOV edi, DICT_END_POINTER ; ustawiamy docelowy adres zapisu danych
	MOV [edi], cx ; zapisanie d�ugo�ci s�owa kodowego
	ADD edi, 2 ; ustawienie si� na miejsce zapisu s�owa kodowego
	CLD
	REP MOVSB ; skopiowanie s�owa kodowego do s�ownika

	MOV DICT_END_POINTER, edi ; ustalenie nowego ko�ca s�ownika

	POPAD ; odtworzenie rejestr�w
	RET
dictAddCodeword ENDP

; ============================================================================;
; Procedura inicjuje s�ownik alfabetem.
; ============================================================================;
dictInitAlphabet PROC
	; zachowanie rejestr�w
	PUSHAD
	MOV ecx, [ebx + 32] ; odczytanie ilo�ci znak�w w alfabecie
	MOV esi, [ebx + 28] ; wska�nik na alfabet

alphabetLoop:
	INVOKE dictAddCodeword, esi, 1 ; dodaj pojedynczy znak alfabetu do s�ownika
	ADD esi, 1
	LOOP alphabetLoop

	POPAD
	RET
dictInitAlphabet ENDP

; ============================================================================;
; Procedura inicjuj�ca zmienne DICT_POINTER oraz DICT_END_POINTER
; ============================================================================;
dictInit PROC
	MOV eax, [ebx + 20] ; ustawiamy wska�nik na miejsce dla s�ownika
	MOV DICT_POINTER, eax
	MOV DICT_END_POINTER, eax ; pierwszym wolnym bajtem jest pocz�tek
	MOV eax, [ebx + 24] ; ustawiamy maksymalny rozmiar danych w s�owniku
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
	MOV ebx, 0 ; aktualny numer (kod) elementu w s�owniku
	MOV ecx, 0 ; zerujemy ecx - b�dzie s�u�y� za d�ugo�� aktualnego elementu w s�owniku (jego dolne 2 bajty)
	
	; p�tla przeszukiwania kolejnych element�w s�ownika
	CLD
searchLoop:
	MOV eax, edi ; zapami�tanie adresu na wypadek znalezienia
	MOV cx, [edi] ; pobieramy d�ugo�� elementu
	ADD edi, 2
	CMP ecx, edx ; sprawdzamy, czy d�ugo�ci s� r�wne
	JNE skip ; nie ma sensu por�wnywa� ci�g�w o r�nych d�ugo�ciach - przeskoczenie do kolejnego elementu

	;p�tla por�wnania danego elementu
	PUSH esi ; zapami�tanie adresu �r�d�owego do kolejnego por�wnania (z kolejnym elementem)
cmpLoop:
	CMPSB
	LOOPE cmpLoop
	POP esi ; przywr�cenie pierwotnego esi dla por�wnania z kolejnym elementem
	JE equal ; je�li ca�y ci�g si� zgadza

skip: ; pomini�cie niepor�wnanych bajt�w
	ADD edi, ecx ; przesuni�cie edi o d�ugo�� niepor�wnanych danych (lub nawet o ca�y element)
next:
	ADD ebx, 1 ; przechodzimy do kolejnego elementu
	CMP ebx, DICT_COUNT ; sprawdzamy czy nie przeszukali�my ju� wszystkich element�w w s�owniku
	JNZ searchLoop

	; je�li nie znaleziono elementu w s�owniku
	MOV TEMP_CODEWORD_POINTER, 0 ; zwr�cenie zerowego wska�nika (nie znaleziono elementu)
	JMP exit

equal:
	MOV TEMP_CODEWORD_POINTER, eax ; w eax zapami�tano adres na wypadek znalezienia ci�gu
	MOV TEMP_CODE, ebx ; ebx wskazuje na kod znalezionego ci�gu

exit:
	POPAD
	MOV eax, TEMP_CODEWORD_POINTER ; dodatkowe zwr�cenie wska�nika przez eax
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
	MOV ebx, params ; ebx dla kolejnych dw�ch procedur b�dzie wskazywa� struktur� params
	INVOKE dictInit ; inicjalizacja zmiennych s�ownika
	INVOKE dictInitAlphabet ; inicjalizacja s�ownika alfabetem

	MOV esi, [ebx + 0] ; adres danych wej�ciowych
	MOV ecx, [ebx + 4] ; rozmiar danych wej�ciowych (ilo�� bajt�w)
	MOV edi, [ebx + 8] ; adres danych wyj�ciowych (skompresowanych)
	MOV CUR_BLOCK_POINTER, edi
	ADD edi, 4 ; 4 bajty rezerwujemy na rozmiar bloku
	MOV edx, 0 ; pocz�tkowa ilo�� wczytanych znak�w - 0

	; g��wna p�tla wczytuj�ca kolejne znaki ze �r�d�a
dataLoop:
	ADD edx, 1 ; wczytaj kolejny znak
	INVOKE dictGetCodewordId ; sprawdzamy czy jest ci�g w s�owniku - przekazanie argument�w przez esi oraz edx
	CMP eax, 0 ; sprawdzamy czy jest w s�owniku
	JNZ next ; je�eli ci�g jest w s�owniku

	; ci�gu nie ma w s�owniku

	; 1 wypisz kod zwi�zany z c i zaktualizuj rozmiar danych skompresowanych
	MOV	eax, LAST_CODE
	MOV [edi], ax
	ADD edi, 2 ; przesuwamy si� o 2 bajty, na kolejn� pozycj� do zapisu kodu
	ADD COMPRESSED_DATA_SIZE, 1

	; 2 dodaj przed�u�ony ostatnio nierozpoznany ci�g do s�ownika
	INVOKE dictAddCodeword, esi, dx

	; 3 prefiksem do kolejnego ci�gu, jest ostatni znak z dodanego ci�gu do s�ownika
	ADD esi, edx ; dodajemy d�ugo�� ostatniego ci�gu
	SUB esi, 1 ; cofamy si� o jeden znak
	MOV edx, 1 ; ustawiamy d�ugo�� nowego analizowanego ci�gu na 1

	; analiza czy s�ownik osi�gn�� limit rozmiaru (DICT_MAX_SIZE)
	MOV eax, DICT_CUR_SIZE
	CMP DICT_MAX_SIZE, eax
	JG ok ; rozmiar jeszcze nie przekroczy�

	; analiza czy s�ownik osi�gn�� limit element�w (na sztywno ustawiony na 65536 element�w)
	CMP DICT_COUNT, 65536
	JG ok ; nie osi�gn�� limitu element�w

	; s�ownik osi�gn�� limit, trzeba wyczy�ci�, utworzy� nowy, i rozpocz�� nowy blok
	; 1. inicjalizacja nowego s�ownika
	MOV ebx, params ; ebx dla kolejnych dw�ch procedur b�dzie wskazywa� struktur� params
	INVOKE dictInit ; inicjalizacja zmiennych s�ownika
	INVOKE dictInitAlphabet ; inicjalizacja s�ownika alfabetem

	; 2. zapisanie rozmiaru bloku, dodanie do sumarycznego rozmiaru
	MOV ebx, CUR_BLOCK_POINTER ; pobranie adresu pocz�tku bloku
	MOV eax, COMPRESSED_DATA_SIZE
	MOV [ebx], eax
	ADD SUMMARY_COMPRESSED_DATA_SIZE, eax

	; 3. zwi�kszenie numeracji ilo�ci blok�w
	ADD BLOCK_COUNT, 1
	
	; 4. ustawienie nowego bloku
	ADD eax, eax ; otrzymanie ilo�ci bajt�w danych skompresowanych
	ADD eax, 4 ; dodanie 4 bajt�w rozmiaru z pocz�tku bloku
	ADD CUR_BLOCK_POINTER, eax ; ustalenie nowego pocz�tku bloku
	MOV edi, CUR_BLOCK_POINTER ; nowe miejsce zapisu kod�w
	ADD edi, 4 ; zarezerwowanie 4 bajt�w na rozmiar nowego bloku
	MOV COMPRESSED_DATA_SIZE, 0 ; wyzerowanie aktualnego rozmiaru danych skompresowanych

ok:
	; aktualizujemy ostatnio rozpoznany ci�g
	INVOKE dictGetCodewordId ; musi by� w s�owniku, poniewa� jest to pojedynczy znak alfabetu
	;MOV LAST_CODEWORD_POINTER, TEMP_CODEWORD_POINTER

next:
	;MOV LAST_CODEWORD_POINTER, TEMP_CODEWORD_POINTER ; zapami�taj ostatnio rozpoznany ci�g
	MOV eax, TEMP_CODE
	MOV LAST_CODE, eax ; zapami�taj ostatnio rozpoznany kod

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