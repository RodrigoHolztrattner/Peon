////////////////////////////////////////////////////////////////////////////////
// Filename: PeonMap.h
////////////////////////////////////////////////////////////////////////////////
#pragma once

//////////////
// INCLUDES //
//////////////
#include "PeonConfig.h"
#include "PeonSystem.h"

#include <iostream>

#include <map>
#include <atomic>
#include <mutex>

/////////////
// DEFINES //
/////////////

////////////
// GLOBAL //
////////////

/*
	=> Map container, attempt 2.0

	=> Description (pt-br until I got time to translate my notes to english)

		* Introdu��o:

			- Continua��o da vers�o 1.0 usando uma nova ideia (bem mais complicada).

		* Ideia:

			PS: N�o cheguei a incluir um m�todo de find() mas o m�todo de atualiza��o existe e � chamado de forma implicita conforma ser� 
			descrito abaixo.

			- Seguimos a ideia da vers�o 1.0 mas com alguns diferenciais, primeiro n�s usaremos 2 maps internos e 2 listas, sempre estaremos
			usando o mesmo index para ler um dos maps e atualizar uma das listas.
			- Usamos v�rias vari�veis de controle, entre elas um contador que indica quantas threads est�o acessando este objeto em quest�o
			de forma concorrente e outra que � incrementada sempre que uma inser��o ou remo��o � feita que basicamente serve como um controle 
			de vers�o do mapa (um valor n�mero que representa uma view).
			- Sempre que vamos adicionar e remover algum elemento, chamamos uma fun��o auxiliar, a UpdateAndSwap().
			- A fun��o UpdateAndSwap() no primeiro momento vai realizar um try_lock() num mutex, garantindo que periodicamente uma thread
			tenha acesso ao seguimento da fun��o e retornando no caso de falha (antes do retorno realizamos um outro lock e unlock mas isso
			ser� explicado adiante).
			- Suponto que conseguimos o lock, salvamos a vers�o atual do map em uma vari�vel. Usando uma vari�vel que indica a ultima posi��o 
			da lista de pend�ncia usada por esse m�todo, n�s seguimos adicionando/removendo os elementos da lista de pend�ncia atual no OUTRO 
			map (o que n�o est� em uso) sempre atualizando essa vari�vel que indica a ultima posi��o v�lida para a mais recente atualizada.
			- Chegando no final da lista, verificamos se existe a necessidade de realizar o swap do index de uso atual, verificamos se a 
			diferen�a dos dois maps internos � muito grande (al�m se no m�nimo temos um tamanho x), caso seja necess�rio, realizamos o swap.
			- Para realizar o swap, � necess�rio garantirmos que somos a �nica tread dentro do objeto, para isso antes de ler a vari�vel 
			que indica a quantidade de threads, realizamos um lock em um outro mutex, que basicamente serve para garantir que nenhuma outra 
			thread possa avan�ar com uma inser��o ou dele��o enquanto realizamos o swap, garantindo essa unicidade e tamb�m que a vers�o
			do objeto ainda � a mesma (n�o ocorreu nenhuma inser��o ou remo��o ap�s o fim de atualiza��o do outro map) n�s efetivamos o swap
			trocando o index em uso e logo liberando o mutex.
			- O map que era usado como principal antes agora passar a ser o "OUTRO" map, ele � resetado e copiamos todos os novos objetos do
			map mais atualizado para ele (lembrando que neste caso podemos fazer isso j� que a leitura neste momento � thread-safe).
			- A lista antiga � zerada, desalocando todos os objetos pertencentes a ela. Ajustamos tamb�m a nova vari�vel que indica a ultima
			posi��o da lista de pendencia usada para o in�cio da nova lista em uso.
			- Liberamos por fim o primeiro mutex (aquele que foi utilizano no try_lock()) e seguimos com a opera��o que deveria ser realizada
			em primeiro lugar.

		* Detalhes:

			- Acabamos fazendo com que quase sempre tentemos realizar o swap, isso tem grande impacto na performance mas eu n�o achei uma ideia
			melhor de realizar isso at� o momento.
			- Utilizamos a aloca��o de mem�ria do proprio sistema, nesse caso aloca��es desnecess�rias n�o s�o um problema.
			- Uma fun��o de find() possivelmente ir� passar pelo map interno + elementos da lista, sendo que para estes elementos � necess�rio 
			uma compara��o um a um (aqui tamb�m existe um impacto na performance).
			- Utilizamos 1 mutex realmente bloqueante, n�o otimizei o c�digo at� o momento de forma a remove-lo e acredito que isso seja poss�vel
			sim, pois definitivamente causa um grande impacto na performance.

		* Resultado:

			- Incrivelmente eu tive melhores resultados com este map do que usando um std::map + mutexes, o ganho n�o foi muito grande, algo em 
			torno de 50% mas geralmente varia bastante.
			- O principal ganho se mostra com a utiliza��o de mais threads, usando 32 por exemplo o ganho foi quase de 30 vezes, aparentemente 
			essa solu��o escala bem numa arquitetura com muitas threads.
*/

///////////////
// NAMESPACE //
///////////////

// __InternalPeon
PeonNamespaceBegin(__InternalPeon)

////////////////////////////////////////////////////////////////////////////////
// Class name: PeonMap
////////////////////////////////////////////////////////////////////////////////
typedef uint32_t ObjectType;	// TODO: Replace this with template parameters
typedef uint32_t Key;			// TODO: Replace this with template parameters
class PeonMap
{
private:

	// The list item operation type
	enum class ItemOperation
	{
		Delete,
		Insert
	};

	// The list item type
	struct ListItem
	{
		// The key
		Key key;

		// The object
		ObjectType object;

		// The item operation
		ItemOperation operation;

		// The next list item
		std::atomic<ListItem*> next;
	};

public:
	PeonMap();
	PeonMap(const PeonMap&);
	~PeonMap();

//////////////////
// MAIN METHODS //
public: //////////

	// Insert a new element
	bool Insert(Key _key, ObjectType _object);

	// Remove an element by key
	bool Remove(Key _key);

	// Return the size
	uint32_t Size();

	// Print the entire map
	void PrintMap(bool _onlyCollisions = false);

private:

	// Insert a new item on the pendency list or check for the return indicator
	bool TryInsertItem(Key _key, ObjectType _object, ItemOperation _itemOperation, bool _returnInitialCondition, bool _returnCondition);

	// Update the other index and try to perform a swap with the current index
	void UpdateAndSwap();

///////////////
// VARIABLES //
private: //////

	// The thread-safe internal map
	std::map<Key, ListItem*, std::less<Key>, PeonAllocator<std::pair<const Key, ListItem*>>> m_InternalMap[2];

	// The pendency item list
	std::atomic<ListItem*> m_PendencyList[2];

	// The current index in use
	std::atomic<uint32_t> m_CurrentIndex;

	// The swap mutex
	std::mutex m_SwapMutex;
	std::mutex m_SwapControlMutex;

	// The last updated pendency item
	std::atomic<ListItem*>* m_LastUpdatedPendencyItem;

	// The number of threads concurrently accessing this object
	std::atomic<uint32_t> m_TotalConcurrentAccesses;

	// The object version (incremented on each insert/deletion)
	std::atomic<uint32_t> m_ObjectVersion;





	// The item count control variable
	std::atomic<uint32_t> m_ItemCount;

	// The list element count
	std::atomic<uint32_t> m_ListItemCount;

	// The count of collisions for insertions and deletions
	std::atomic<uint32_t> m_CollisionCount;
};

// __InternalPeon
PeonNamespaceEnd(__InternalPeon)
