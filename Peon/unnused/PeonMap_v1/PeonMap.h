////////////////////////////////////////////////////////////////////////////////
// Filename: PeonMap.h
////////////////////////////////////////////////////////////////////////////////
#pragma once

//////////////
// INCLUDES //
//////////////
#include "PeonConfig.h"
#include "PeonMemoryAllocator.h"

#include <iostream>

#include <map>
#include <atomic>

/////////////
// DEFINES //
/////////////

////////////
// GLOBAL //
////////////

/*
	=> Map container, attempt 1.0

	=> Description (pt-br until I got time to translate my notes to english)

		* Introdu��o:

			- Tudo come�ou comigo pensando em maneiras de expandir o PeonSystem, a ideia era abusar do sistema para o lado de containers.
			- Essa � a primeira vers�o dessa classe, existe pelo menos uma nova vers�o (que deve estar na mesma pasta que essa).
		
		* Ideia:

			PS: N�o cheguei a incluir um m�todo de find() e muito menos terminar o que ser� explicado adiante, percebi que o funcionamento
			estava lento demais e resolvi serguir com uma nova ideia (vers�o 2.0).

			- Construir um container no estilo "map" que fosse thread-safe, o mais pr�ximo poss�vel de lock-free e com uma velocidade boa
			quando comparado com um std::map + mutex.
			- A ideia foi usar o fato de o sistema precisar chamar o m�todo de ResetWorkerFrame() para somente ent�o realizar atualiza��es
			nesta classe, portanto usar um std::map interno e uma lista de pend�ncias.
			- Quando uma thread deseja inserir ou remover um objeto deste map, ela na verdade cria uma pend�ncia para esta a��o, que � um 
			item de uma lista e tenta adicion�-lo na lista de pend�ncias (usando um sistema lock-free com compare_and_swap()).
			- Para procurar por um objeto, primeiro varreriamos o map interno e depois passariamos a verificar a lista de pend�ncia, sempre
			cuidando que se existe uma pend�ncia de inser��o, a mesma pode ser negada com uma de dele��o e vice-versa.
			- Quando o m�todo ResetWorkerFrame() fosse chamado no PeonSystem, todos os containers que tivessem algum objeto em sua lista de
			pend�ncia moveriam estes para o map interno.
			- Porque foi escolhido uma lista? Porque a ideia era que n�o ocorressem muitas modifica��es por frame ao ponto de tornar a 
			travessia da lista custosa, mas foi o que acabou ocorrendo (sem falar que o compare_and_swap() come�ou a se tornar invi�vel 
			conforme o tamanho da lista aumentasse).

			- Continuei com a vers�o 2.0.
*/

///////////////
// NAMESPACE //
///////////////

// __InternalPeon
PeonNamespaceBegin(__InternalPeon)

// We know the job system
class PeonSystem;
class PeonWorker;

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

///////////////
// VARIABLES //
private: //////

	// The thread-safe internal map
	std::map<Key, ListItem*> m_InternalMap;

	// The pendency item list
	std::atomic<ListItem*> m_PendencyList;

	// The item count control variable
	std::atomic<uint32_t> m_ItemCount;

	// The list element count
	std::atomic<uint32_t> m_ListItemCount;

	// The count of collisions for insertions and deletions
	std::atomic<uint32_t> m_CollisionCount;

	// The number of times we had zero elements
	std::atomic<uint32_t> m_ZeroElementsCount;
};

// __InternalPeon
PeonNamespaceEnd(__InternalPeon)
