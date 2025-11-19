// Dossier2Git.cpp  L'exécution du programme commence et se termine à cet endroit.
// © Patrice Waechter-Ebling 2024
//
#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <shlobj.h>
#include <shlwapi.h>

#pragma warning (disable:4996)
#pragma warning (disable:6063)
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

static char ClefJeton[] = "";
static char NomDepot[0xfe];
static char NomLien[0x104];//MAX_PATH
static std::wstring s2ws(const std::string& str);
static void PasserCmdGIT();
static int CreerVariableTocken(char* Clef);
static void ChoisirDossier();
static int CreerDepot(char* nom);
static void NouveauProjet();

static std::wstring s2ws(const std::string& str) {
	int len;
	int slen = (int)str.length() + 1;
	len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), slen, 0, 0);
	std::wstring r(len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), slen, &r[0], len);
	return r;
}
static void PasserCmdGIT() {
	char tmp[0xFC];
	SYSTEMTIME st;
	GetLocalTime(&st);
	wsprintfA(tmp, "echo #%s  >> README.md", NomDepot);
	system(tmp);
	wsprintfA(tmp, "git init");
	system(tmp);
	wsprintfA(tmp, "git add *.*");
	system(tmp);
	wsprintfA(tmp, "git commit -m %.4d-%.2d-%.2d %.2d:%.2d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
	system(tmp);
	wsprintfA(tmp, "git branch -M main");
	system(tmp);
	wsprintfA(tmp, "git remote add origin %s", NomLien);
	system(tmp);
	wsprintfA(tmp, "git push -u origin main");
	system(tmp);
}
static int CreerVariableTocken(char* Clef) { return SetEnvironmentVariableA("GITHUB_TOKEN", Clef); }
static void ChoisirDossier() {
	BROWSEINFOA bi = { 0 };
	bi.lpszTitle = "Choisissez un dossier";
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
	if (pidl != nullptr) {
		char path[MAX_PATH];
		if (SHGetPathFromIDListA(pidl, path)) {
			std::cout << "Dossier choisi: " << path << std::endl;
			strcpy(NomDepot, PathFindFileNameA(path));
			std::cout << "Nom du depot: " << NomDepot << std::endl;
			wsprintfA(NomLien, "https://github.com/patdev-qc-ca/%s.git", NomDepot);
			std::cout << "Nom du lien: " << NomLien << std::endl;
			CreerDepot(NomDepot);
		}
		CoTaskMemFree(pidl);
	}
}
static int CreerDepot(char* nom) {
	std::string Jeton(ClefJeton);
	char InstructionJSON[256];
	wsprintfA(InstructionJSON, "({ \
		\"name\": \"%s\",\
		\"description\": \"Dépot créé par Dossier2Git pour %s\",\
		\"private\": false,\
		\"auto_init\": true})", nom, nom);
	HINTERNET hConnexion = WinHttpOpen(L"Dossier2Git", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hConnexion) {
		std::cerr << "Echec lors de l'execution de l'api WinHttpOpen\n";
		return 1;
	}
	HINTERNET hConnect = WinHttpConnect(hConnexion, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
	HINTERNET hInternet = WinHttpOpenRequest(hConnect, L"POST", L"/user/repos", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	std::wstring authHeader = L"Authorization: Bearer " + s2ws(Jeton);
	WinHttpAddRequestHeaders(hInternet, L"Accept: application/vnd.github+json", (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);
	WinHttpAddRequestHeaders(hInternet, L"User-Agent: Dossier2Git", (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);
	WinHttpAddRequestHeaders(hInternet, authHeader.c_str(), (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);
	WinHttpAddRequestHeaders(hInternet, L"Content-Type: application/json", (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);
	BOOL bResults = WinHttpSendRequest(hInternet, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)InstructionJSON, (DWORD)strlen(InstructionJSON), (DWORD)strlen(InstructionJSON), 0);
	if (bResults)
		bResults = WinHttpReceiveResponse(hInternet, NULL);
	if (bResults) {
		DWORD dwSize = 0;
		do {
			DWORD dwDownloaded = 0;
			if (!WinHttpQueryDataAvailable(hInternet, &dwSize))break;
			if (dwSize == 0)break;
			std::string buffer(dwSize, '\0');
			if (!WinHttpReadData(hInternet, &buffer[0], dwSize, &dwDownloaded))break;
			std::cout << buffer << std::endl;
		} while (dwSize > 0);
	}
	else {
		std::cerr << "Erreur HTTP\n";
	}
	WinHttpCloseHandle(hInternet);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hConnexion);
	PasserCmdGIT();
	return 0;
}
static void NouveauProjet() {
	ChoisirDossier();
	if (MessageBoxA((HWND)GetStdHandle(STD_INPUT_HANDLE), "Voulez faire un autre projet?", "Dossier2Git", MB_ICONQUESTION | MB_YESNO) == 0x06) {
		ChoisirDossier();
	}
}
int main() {
	CreerVariableTocken(ClefJeton);
	std::cout << " Dossier2Git\tv:1.02\t(C) Patrice Waechter-Ebling 2024" << std::endl;
	std::string Jeton(ClefJeton);
	std::cout << "Chiffrage SSH: " << Jeton.substr(13, 32) << std::endl;
	NouveauProjet();
	return 0;
}
