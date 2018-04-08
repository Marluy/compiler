#include <stdlib.h>
#include "ArbreAbstrait.h"
#include "Symbole.h"
#include "SymboleValue.h"
#include "Exceptions.h"
#include <iostream>
#include <typeinfo>
#include <map>

using namespace std;

////////////////////////////////////////////////////////////////////////////////
// NoeudSeqInst
////////////////////////////////////////////////////////////////////////////////

NoeudSeqInst::NoeudSeqInst() : m_instructions() {
}

int NoeudSeqInst::executer() {
	for (unsigned int i = 0; i < m_instructions.size(); i++)
		m_instructions[i]->executer(); // on exécute chaque instruction de la séquence

	return 0; // La valeur renvoyée ne représente rien !
}

void NoeudSeqInst::traduitEnCPP(ostream & cout, unsigned int indentation) const {
	for (unsigned int i = 0; i < m_instructions.size(); i++) {
		cout << setw(indentation) << "\t";
		m_instructions[i]->traduitEnCPP(cout, indentation);
		cout << endl;
	}
}

void NoeudSeqInst::ajoute(Noeud* instruction) {
	if (instruction != NULL) m_instructions.push_back(instruction);
}

////////////////////////////////////////////////////////////////////////////////
// NoeudAffectation
////////////////////////////////////////////////////////////////////////////////

NoeudAffectation::NoeudAffectation(Noeud* variable, Noeud* expression)
: m_variable(variable), m_expression(expression) {
}

int NoeudAffectation::executer() {
	int valeur = m_expression->executer(); // On exécute (évalue) l'expression
	((SymboleValue*) m_variable)->setValeur(valeur); // On affecte la variable

	return 0; // La valeur renvoyée ne représente rien !
}

void NoeudAffectation::traduitEnCPP(ostream & cout, unsigned int indentation) const {

	if (typeid (*m_expression) == typeid (NoeudOperateurBinaire)
			&&(((NoeudOperateurBinaire*) m_expression)->getOperateur() == "++"
			|| ((NoeudOperateurBinaire*) m_expression)->getOperateur() == "--")) {
		if (((NoeudOperateurBinaire*) m_expression)->getOperandeDroit() == NULL) {
			m_variable->traduitEnCPP(cout, 0);
			cout << ((NoeudOperateurBinaire*) m_expression)->getOperateur();
		} else {
			cout << ((NoeudOperateurBinaire*) m_expression)->getOperateur();
			m_variable->traduitEnCPP(cout, 0);
		}
	}
	else {
		m_variable->traduitEnCPP(cout, 0);
		cout << " = ";
		m_expression->traduitEnCPP(cout, 0);
	}
	cout << ";";
}

////////////////////////////////////////////////////////////////////////////////
// NoeudOperateurBinaire
////////////////////////////////////////////////////////////////////////////////

NoeudOperateurBinaire::NoeudOperateurBinaire(Symbole operateur, Noeud* operandeGauche, Noeud* operandeDroit)
: m_operateur(operateur), m_operandeGauche(operandeGauche), m_operandeDroit(operandeDroit) {
}

int NoeudOperateurBinaire::executer() {
	int og, od, valeur;
	if (m_operandeGauche != NULL) og = m_operandeGauche->executer(); // On évalue l'opérande gauche
	if (m_operandeDroit != NULL) od = m_operandeDroit->executer(); // On évalue l'opérande droit
	// Et on combine les deux opérandes en fonctions de l'opérateur
	if (this->m_operateur == "+")valeur = (og + od);
	else if (this->m_operateur == "-") valeur = (og - od);
	else if (this->m_operateur == "++") {
		if (m_operandeGauche != NULL) valeur = (og + 1);
		else valeur = (od + 1);
	} else if (this->m_operateur == "--") {
		if (m_operandeGauche != NULL) valeur = (og - 1);
		else valeur = (od - 1);
	} else if (this->m_operateur == "*") valeur = (og * od);
	else if (this->m_operateur == "==") valeur = (og == od);
	else if (this->m_operateur == "!=") valeur = (og != od);
	else if (this->m_operateur == "<") valeur = (og < od);
	else if (this->m_operateur == ">") valeur = (og > od);
	else if (this->m_operateur == "<=") valeur = (og <= od);
	else if (this->m_operateur == ">=") valeur = (og >= od);
	else if (this->m_operateur == "et") valeur = (og && od);
	else if (this->m_operateur == "ou") valeur = (og || od);
	else if (this->m_operateur == "non") valeur = (!og);
	else if (this->m_operateur == "/") {
		if (od == 0) throw DivParZeroException();
		valeur = og / od;
	}
	return valeur; // On retourne la valeur calculée
}

void NoeudOperateurBinaire::traduitEnCPP(ostream & cout, unsigned int indentation) const {

	// A gauche
	if (m_operandeGauche != NULL) {
		// On regarde si l'operation est prioritaire sur la precedente
		// Si oui, on mettera des parenthèses

		bool bracket = typeid (*m_operandeGauche) == typeid (NoeudOperateurBinaire) &&
				getPriority_Cpp(m_operateur) > getPriority_Cpp(((NoeudOperateurBinaire*) m_operandeGauche)->getOperateur());

		if (bracket) cout << "(";
		m_operandeGauche->traduitEnCPP(cout, indentation);
		if (bracket) cout << ")";
		cout << " ";
	}

	if (m_operateur == "non") cout << '\010' << "!";
	else if (m_operateur == "et") cout << "&&";
	else if (m_operateur == "ou") cout << "||";
	else cout << m_operateur.getChaine();

	// A droite
	if (m_operandeDroit != NULL) {
		cout << " ";
		// On regarde si l'operation est prioritaire sur la precedente
		// Si oui, on mettera des parenthèses

		bool bracket = typeid (*m_operandeDroit) == typeid (NoeudOperateurBinaire) &&
				getPriority_Cpp(m_operateur) > getPriority_Cpp(((NoeudOperateurBinaire*) m_operandeDroit)->getOperateur());

		if (bracket) cout << "(";
		m_operandeDroit->traduitEnCPP(cout, indentation);
		if (bracket) cout << ")";
	}
}

////////////////////////////////////////////////////////////////////////////////
// NoeudInstSi
////////////////////////////////////////////////////////////////////////////////

NoeudInstSi::NoeudInstSi(Noeud* condition, Noeud* sequence)
: m_condition(condition), m_sequence(sequence) {
}

int NoeudInstSi::executer() {
	if (m_condition->executer()) m_sequence->executer();
	return 0; // La valeur renvoyée ne représente rien !
}

void NoeudInstSi::traduitEnCPP(ostream & cout, unsigned int indentation) const {
	cout << setw(indentation) << "\t" << "if ("; // Ecrit "if (" avec un décalage de 4*indentation espaces
	m_condition->traduitEnCPP(cout, 0); // Traduit la condition en C++ sans décalage
	cout << ") {" << endl; // Ecrit ") {" et passe à la ligne
	m_sequence->traduitEnCPP(cout, indentation + 9); // Traduit en C++ la séquence avec indentation augmentée
	cout << setw(indentation) << "\t" << "}"; // Ecrit "}" avec l'indentation initiale et passe à la ligne
}

////////////////////////////////////////////////////////////////////////////////
// NoeudInstTantQue
////////////////////////////////////////////////////////////////////////////////

NoeudInstTq::NoeudInstTq(Noeud* condition, Noeud * sequence) : m_condition(condition), m_sequence(sequence) {
}

int NoeudInstTq::executer() {
	while (m_condition->executer()) {
		m_sequence->executer();
	}

	return 0;
}

void NoeudInstTq::traduitEnCPP(ostream & cout, unsigned int indentation) const {
	cout << "while( ";
	m_condition->traduitEnCPP(cout, 0);
	cout << " )" << endl;
	cout << setw(indentation) << "\t" << "{" << endl;
	m_sequence->traduitEnCPP(cout, indentation + 9);
	cout << setw(indentation) << "\t" << "}";
}

////////////////////////////////////////////////////////////////////////////////
// NoeudSiRiche
////////////////////////////////////////////////////////////////////////////////

NoeudInstSiRiche::NoeudInstSiRiche(Noeud* cond, Noeud* seqVrai, Noeud* seqFaux)
: m_condition(cond), m_sequenceVraie(seqVrai), m_sequenceFausse(seqFaux) {
}

int NoeudInstSiRiche::executer() {
	if (m_condition->executer()) {
		m_sequenceVraie->executer();
	} else {
		if (m_sequenceFausse != NULL) m_sequenceFausse->executer();
	}

	return 0;
}

void NoeudInstSiRiche::setSeqFausse(Noeud * noeud) {
	m_sequenceFausse = noeud;
}

void NoeudInstSiRiche::traduitEnCPP(ostream & cout, unsigned int indentation) const {
	cout << "if( ";
	m_condition->traduitEnCPP(cout, 0);
	cout << " )" << endl;
	cout << setw(indentation) << "\t" << "{" << endl;
	m_sequenceVraie->traduitEnCPP(cout, indentation + 9);
	cout << setw(indentation) << "\t" << "}";

	if (m_sequenceFausse != NULL) {
		cout << setw(indentation) << endl << "\t" << "else ";

		if (typeid (*m_sequenceFausse) == typeid (NoeudInstSiRiche)) {
			m_sequenceFausse->traduitEnCPP(cout, indentation);
		} else {
			cout << endl << setw(indentation) << "\t" << "{" << endl;
			m_sequenceFausse->traduitEnCPP(cout, indentation + 9);
			cout << setw(indentation) << "\t" << "}";
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
// NoeudInstTantQue
////////////////////////////////////////////////////////////////////////////////

NoeudInstRpt::NoeudInstRpt(Noeud* condition, Noeud * sequence) : m_condition(condition), m_sequence(sequence) {
}

int NoeudInstRpt::executer() {
	do {
		m_sequence->executer();
	} while (!m_condition->executer());

	return 0;
}

void NoeudInstRpt::traduitEnCPP(ostream & cout, unsigned int indentation) const {
	cout << "do" << endl;
	cout << setw(indentation) << "\t" << "{" << endl;
	m_sequence->traduitEnCPP(cout, indentation + 9);
	cout << setw(indentation) << "\t" << "}" << endl;
	cout << setw(indentation) << "\t" << "while( ";
	m_condition->traduitEnCPP(cout, 0);
	cout << " );";
}

////////////////////////////////////////////////////////////////////////////////
// NoeudInstPour
////////////////////////////////////////////////////////////////////////////////

NoeudInstPr::NoeudInstPr(Noeud* initialisation, Noeud* condition, Noeud* incrementation, Noeud* sequence)
: m_initialisation(initialisation), m_condition(condition), m_incrementation(incrementation), m_sequence(sequence) {
}

int NoeudInstPr::executer() {
	if (m_initialisation != NULL) m_initialisation->executer();
	while (m_condition->executer()) {
		m_sequence->executer();
		if (m_incrementation != NULL) m_incrementation->executer();
	}

	return 0;
}

void NoeudInstPr::traduitEnCPP(ostream & cout, unsigned int indentation) const {
	cout << "for( ";
	if (m_initialisation != NULL) m_initialisation ->traduitEnCPP(cout, 0);
	cout << '\010' << " ; ";
	m_condition->traduitEnCPP(cout, 0);
	cout << " ; ";
	if (m_incrementation != NULL) m_incrementation->traduitEnCPP(cout, 0);
	cout << '\010' << " )" << endl;
	cout << setw(indentation) << "\t" << "{" << endl;
	m_sequence->traduitEnCPP(cout, indentation + 9);
	cout << setw(indentation) << "\t" << "}";
}

////////////////////////////////////////////////////////////////////////////////
// NoeudInstLire
////////////////////////////////////////////////////////////////////////////////

NoeudInstLire::NoeudInstLire() : m_variables() {
}

int NoeudInstLire::executer() {
	int valeur;

	for (auto var : m_variables) {
		cin >> valeur;
		cin.ignore(256, '\n');
		((SymboleValue*) var)->setValeur(valeur);
	}

	return 0;
}

void NoeudInstLire::ajoute(Noeud* var) {
	m_variables.push_back(var);
}

void NoeudInstLire::traduitEnCPP(ostream & cout, unsigned int indentation) const {

	cout << m_variables.size() << endl;

	for (auto var : m_variables)
		cout << "cin >> " << ((SymboleValue*) var)->getChaine() << "; ";
}

////////////////////////////////////////////////////////////////////////////////
// NoeudInstEcrire
////////////////////////////////////////////////////////////////////////////////

NoeudInstEcrire::NoeudInstEcrire() : m_variables() {
}

int NoeudInstEcrire::executer() {

	for (Noeud* p : m_variables) {
		if (typeid (*p) == typeid (SymboleValue) && *((SymboleValue*) p) == "<CHAINE>") {
			string str(((SymboleValue*) p)->getChaine());
			str = str.erase(0, 1);
			str.pop_back();

			cout << str;
		} else cout << p->executer();
	}
	cout << endl;

	return 0;
}

void NoeudInstEcrire::ajoute(Noeud* var) {
	m_variables.push_back(var);
}

void NoeudInstEcrire::traduitEnCPP(ostream & cout, unsigned int indentation) const {
	cout << "cout";

	for (Noeud* p : m_variables) {
		cout << " << ";

		if (typeid (*p) == typeid (SymboleValue) && *((SymboleValue*) p) == "<CHAINE>") {
			cout << ((SymboleValue*) p)->getChaine();
		} else p->traduitEnCPP(cout, 0);
	}

	cout << " << endl;";
}


////////////////////////////////////////////////////////////////////////////////
// NoeudInstEcrire
////////////////////////////////////////////////////////////////////////////////

NoeudInstSelon::NoeudInstSelon(Noeud* variable)
: m_variable(variable), m_sequences(), m_defaut(NULL) {
}

void NoeudInstSelon::ajoute(Noeud* cas, Noeud* sequence) {
	m_sequences.insert(pair<Noeud*, Noeud*>(cas, sequence));
}

int NoeudInstSelon::executer() {

	auto it = m_sequences.begin();

	while (it != m_sequences.end() && m_variable->executer() != it->first->executer())
		it++;

	if (it != m_sequences.end()) m_defaut->executer();
	else if (m_defaut != NULL) it->second->executer();

	return 0;
}

void NoeudInstSelon::traduitEnCPP(ostream & cout, unsigned int indentation) const {
	cout << "switch(";
	m_variable->traduitEnCPP(cout, 0);
	cout << ")" << endl;
	cout << setw(indentation) << "\t" << "{" << endl;

	for (auto p : m_sequences) {
		cout << setw(indentation + 8) << "\t" << "case ";
		p.first->traduitEnCPP(cout, 0);
		cout << ":" << endl;
		p.second->traduitEnCPP(cout, indentation + 16);
		cout << setw(indentation + 16) << "\t" << "break;" << endl;
	}

	if (m_defaut != NULL) {
		cout << setw(indentation + 8) << "\t" << "default:" << endl;
		m_defaut->traduitEnCPP(cout, indentation + 16);
		cout << setw(indentation + 16) << "\t" << "break;" << endl;
	}
	cout << setw(indentation) << "\t" << "}" << endl;
}

int Noeud::getPriority_Cpp(Symbole s) {
	if (s == "non") return 7;
	else if (s == "*" || s == "/") return 6;
	else if (s == "+" || s == "-") return 5;
	else if (s == ">=" || s == ">" ||
			s == "<=" || s == "<") return 4;
	else if (s == "==" || s == "!=") return 3;
	else if (s == "et") return 2;
	else if (s == "ou") return 1;
	else return 0;
}