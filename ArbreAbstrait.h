#ifndef ARBREABSTRAIT_H
#define ARBREABSTRAIT_H

// Contient toutes les déclarations de classes nécessaires
//  pour représenter l'arbre abstrait

#include <vector>
#include <iostream>
#include <iomanip>
#include <typeinfo>
using namespace std;

#include "Symbole.h"
#include "Exceptions.h"

////////////////////////////////////////////////////////////////////////////////

class Noeud {
	// Classe abstraite dont dériveront toutes les classes servant à représenter l'arbre abstrait
	// Remarque : la classe ne contient aucun constructeur
public:
	virtual int executer() = 0; // Méthode pure (non implémentée) qui rend la classe abstraite

	virtual void ajoute(Noeud* instruction) {
		throw OperationInterditeException();
	}

	virtual void traduitEnCPP(ostream & cout, unsigned int indentation) const = 0;

	static int getPriority_Cpp(Symbole s);

	virtual ~Noeud() {
	} // Présence d'un destructeur virtuel conseillée dans les classes abstraites
};

////////////////////////////////////////////////////////////////////////////////

class NoeudSeqInst : public Noeud {
	// Classe pour représenter un noeud "sequence d'instruction"
	//  qui a autant de fils que d'instructions dans la séquence
public:
	NoeudSeqInst(); // Construit une séquence d'instruction vide

	~NoeudSeqInst() {
	} // A cause du destructeur virtuel de la classe Noeud
	int executer(); // Exécute chaque instruction de la séquence
	void traduitEnCPP(ostream & cout, unsigned int indentation) const;
	void ajoute(Noeud* instruction); // Ajoute une instruction à la séquence

private:
	vector<Noeud *> m_instructions; // pour stocker les instructions de la séquence
};

////////////////////////////////////////////////////////////////////////////////

class NoeudAffectation : public Noeud {
	// Classe pour représenter un noeud "affectation"
	//  composé de 2 fils : la variable et l'expression qu'on lui affecte
public:
	NoeudAffectation(Noeud* variable, Noeud* expression); // construit une affectation
	void traduitEnCPP(ostream & cout, unsigned int indentation) const;

	~NoeudAffectation() {
	} // A cause du destructeur virtuel de la classe Noeud
	int executer(); // Exécute (évalue) l'expression et affecte sa valeur à la variable

private:
	Noeud* m_variable;
	Noeud* m_expression;
};

////////////////////////////////////////////////////////////////////////////////

class NoeudOperateurBinaire : public Noeud {
	// Classe pour représenter un noeud "opération binaire" composé d'un opérateur
	//  et de 2 fils : l'opérande gauche et l'opérande droit
public:
	NoeudOperateurBinaire(Symbole operateur, Noeud* operandeGauche, Noeud* operandeDroit);
	// Construit une opération binaire : operandeGauche operateur OperandeDroit

	~NoeudOperateurBinaire() {
	} // A cause du destructeur virtuel de la classe Noeud
	int executer(); // Exécute (évalue) l'opération binaire)
	void traduitEnCPP(ostream & cout, unsigned int indentation) const;

private:

	inline std::string getOperateur() const {
		return m_operateur.getChaine();
	}

	Symbole m_operateur;
	Noeud* m_operandeGauche;
	Noeud* m_operandeDroit;
};

////////////////////////////////////////////////////////////////////////////////

class NoeudInstSi : public Noeud {
	// Classe pour représenter un noeud "instruction si"
	//  et ses 2 fils : la condition du si et la séquence d'instruction associée
public:
	NoeudInstSi(Noeud* condition, Noeud* sequence);
	// Construit une "instruction si" avec sa condition et sa séquence d'instruction

	~NoeudInstSi() {
	} // A cause du destructeur virtuel de la classe Noeud
	int executer(); // Exécute l'instruction si : si condition vraie on exécute la séquence
	void traduitEnCPP(ostream & cout, unsigned int indentation) const;

private:
	Noeud* m_condition;
	Noeud* m_sequence;
};

////////////////////////////////////////////////////////////////////////////////

class NoeudInstTq : public Noeud {
public:
	NoeudInstTq(Noeud* condition, Noeud * sequence);
	int executer();
	void traduitEnCPP(ostream & cout, unsigned int indentation) const;

private:
	Noeud* m_condition;
	Noeud* m_sequence;
};

////////////////////////////////////////////////////////////////////////////////

class NoeudInstSiRiche : public Noeud {
public:
	NoeudInstSiRiche(Noeud* cond, Noeud* seqVrai, Noeud* seqFaux);
	int executer();
	void setSeqFausse(Noeud * noeud);
	void traduitEnCPP(ostream & cout, unsigned int indentation) const;

private:
	Noeud* m_condition;
	Noeud* m_sequenceVraie;
	Noeud* m_sequenceFausse;

};

////////////////////////////////////////////////////////////////////////////////

class NoeudInstRpt : public Noeud {
public:
	NoeudInstRpt(Noeud* condition, Noeud * sequence);
	int executer();
	void traduitEnCPP(ostream & cout, unsigned int indentation) const;

private:
	Noeud* m_condition;
	Noeud* m_sequence;
};

////////////////////////////////////////////////////////////////////////////////

class NoeudInstPr : public Noeud {
public:
	NoeudInstPr(Noeud* initialisation, Noeud* condition, Noeud* incrementation, Noeud* sequence);
	int executer();
	void traduitEnCPP(ostream & cout, unsigned int indentation) const;

private:
	Noeud* m_initialisation;
	Noeud* m_condition;
	Noeud* m_incrementation;
	Noeud* m_sequence;
};

////////////////////////////////////////////////////////////////////////////////

class NoeudInstLire : public Noeud {
public:
	NoeudInstLire();
	int executer();
	void traduitEnCPP(ostream & cout, unsigned int indentation) const;
	void ajoute(Noeud* var);

private:
	vector<Noeud*> m_variables;
};

////////////////////////////////////////////////////////////////////////////////

class NoeudInstEcrire : public Noeud {
public:
	NoeudInstEcrire();
	int executer();
	void traduitEnCPP(ostream & cout, unsigned int indentation) const;
	void ajoute(Noeud* var);

private:
	vector<Noeud*> m_variables;
};
#endif /* ARBREABSTRAIT_H */
