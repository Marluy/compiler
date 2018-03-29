#include <stdlib.h>
#include "ArbreAbstrait.h"
#include "Symbole.h"
#include "SymboleValue.h"
#include "Exceptions.h"
#include <iostream>

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
    if (this->m_operateur == "+") valeur = (og + od);
    else if (this->m_operateur == "-") valeur = (og - od);
    else if (this->m_operateur == "*") valeur = (og * od);
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


////////////////////////////////////////////////////////////////////////////////
// NoeudInstTantQue
////////////////////////////////////////////////////////////////////////////////

NoeudInstSiRiche::NoeudInstSiRiche(Noeud* cond, Noeud* seqVrai, Noeud* seqFaux)
: m_condition(cond), m_sequenceVraie(seqVrai), m_sequenceFausse(seqFaux) {
}

int NoeudInstSiRiche::executer() {
    if (m_condition->executer()) {
        m_sequenceVraie->executer();
    } else {
        m_sequenceFausse->executer();
    }

    return 0;
}

void NoeudInstSiRiche::setSeqFausse(Noeud * noeud) {
    m_sequenceFausse = noeud;
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

////////////////////////////////////////////////////////////////////////////////
// NoeudInstPour
////////////////////////////////////////////////////////////////////////////////

NoeudInstLire::NoeudInstLire()
: m_variables() {
}

int NoeudInstLire::executer() {
    int valeur;

    for (auto var : m_variables) {
        cin >> valeur;
        cin.ignore(256, '\n');
        ((SymboleValue*) var)->setValeur(valeur);
        ajoute(var);
    }
}

void NoeudInstLire::ajoute(Noeud* var){
    m_variables.push_back(var);
}
