#include "Interpreteur.h"
#include <stdlib.h>
#include <iostream>
using namespace std;

Interpreteur::Interpreteur(ifstream & fichier) :
m_lecteur(fichier), m_table(), m_arbre(NULL) {
}

void Interpreteur::analyse() {
	m_arbre = programme(); // on lance l'analyse de la première règle
}

void Interpreteur::traduitEnCPP(ostream & cout) const {
	cout << "#include <iostream>" << endl << endl;
	cout << "int main()" << endl << "{" << endl;

	cout << setw(1) << "\t" << "int ";
	for (int i(0); i < m_table.getTaille(); i++)
		if (m_table[i] == "<VARIABLE>") cout << m_table[i].getChaine() << "(0), ";
	cout << '\010' << '\010' << ";" << endl << endl;

	getArbre()->traduitEnCPP(cout, 1);

	cout << endl << setw(1) << "\t" << "return 0;" << endl;
	cout << "}" << endl;
}

void Interpreteur::tester(const string & symboleAttendu) const throw (SyntaxeException) {// Teste si le symbole courant est égal au symboleAttendu... Si non, lève une exception
	static char messageWhat[256];
	if (m_lecteur.getSymbole() != symboleAttendu) {
		sprintf(messageWhat,
				"Ligne %d, Colonne %d - Erreur de syntaxe - Symbole attendu : %s - Symbole trouvé : %s",
				m_lecteur.getLigne(), m_lecteur.getColonne(),
				symboleAttendu.c_str(), m_lecteur.getSymbole().getChaine().c_str());
		throw SyntaxeException(messageWhat);
	}
}

void Interpreteur::testerEtAvancer(const string & symboleAttendu) throw (SyntaxeException) {// Teste si le symbole courant est égal au symboleAttendu... Si oui, avance, Sinon, lève une exception
	tester(symboleAttendu);
	m_lecteur.avancer();
}

void Interpreteur::erreur(const string & message) const throw (SyntaxeException) {// Lève une exception contenant le message et le symbole courant trouvé
	// Utilisé lorsqu'il y a plusieurs symboles attendus possibles...
	static char messageWhat[256];
	sprintf(messageWhat,
			"Ligne %d, Colonne %d - Erreur de syntaxe - %s - Symbole trouvé : %s",
			m_lecteur.getLigne(), m_lecteur.getColonne(), message.c_str(), m_lecteur.getSymbole().getChaine().c_str());
	throw SyntaxeException(messageWhat);
}

Noeud* Interpreteur::programme() {// <programme> ::= procedure principale() <seqInst> finproc FIN_FICHIER
	testerEtAvancer("procedure");
	testerEtAvancer("principale");
	testerEtAvancer("(");
	testerEtAvancer(")");
	Noeud* sequence = seqInst();
	testerEtAvancer("finproc");
	tester("<FINDEFICHIER>");
	return sequence;
}

Noeud* Interpreteur::seqInst() {// <seqInst> ::= <inst> { <inst> }
	NoeudSeqInst* sequence = new NoeudSeqInst();
	do {
		try {
			sequence->ajoute(inst());
		} catch (SyntaxeException s) {
			cout << "Exception levée : " << s.what() << endl;
			m_lecteur.avancer();
			m_lecteur.avancer();
		}
	} while (m_lecteur.getSymbole() == "<VARIABLE>" ||
			m_lecteur.getSymbole() == "si" ||
			m_lecteur.getSymbole() == "tantque" ||
			m_lecteur.getSymbole() == "pour" ||
			m_lecteur.getSymbole() == "repeter" ||
			m_lecteur.getSymbole() == "ecrire" ||
			m_lecteur.getSymbole() == "lire");
	// Tant que le symbole courant est un début possible d'instruction...
	// Il faut compléter cette condition chaque fois qu'on rajoute une nouvelle instruction
	return sequence;
}

Noeud* Interpreteur::inst() {// <inst> ::= <affectation> ; | <instSiRiche> | <instTantQue> | <instRepeter> ; | <instPour> | <instEcrire> ; | <instLire> ;
	if (m_lecteur.getSymbole() == "<VARIABLE>") {
		Noeud *affect = affectation();
		try {
			testerEtAvancer(";");
		} catch (SyntaxeException s) {
			cout << "Exception levée : " << s.what() << endl;
			//m_lecteur.avancer();
		}
		return affect;

	} else if (m_lecteur.getSymbole() == "si") return instSiRiche();

	else if (m_lecteur.getSymbole() == "tantque") return instTantQue();

	else if (m_lecteur.getSymbole() == "repeter") {
		Noeud* repet = instRepeter();
		try {
			testerEtAvancer(";");
		} catch (SyntaxeException s) {
			cout << "Exception levée : " << s.what() << endl;
			//m_lecteur.avancer();
		}
		return repet;
	} else if (m_lecteur.getSymbole() == "pour") return instPour();

	else if (m_lecteur.getSymbole() == "ecrire") {
		Noeud* ecrire = instEcrire();
		try {
			testerEtAvancer(";");
		} catch (SyntaxeException s) {
			cout << "Exception levée : " << s.what() << endl;
			//m_lecteur.avancer();
		}
		return ecrire;
	} else if (m_lecteur.getSymbole() == "lire") {
		Noeud* lire = instLire();
		try {
			testerEtAvancer(";");
		} catch (SyntaxeException s) {
			cout << "Exception levée : " << s.what() << endl;
			//m_lecteur.avancer();
		}
		return lire;
	} else erreur("Instruction incorrecte");
}

Noeud* Interpreteur::affectation() {// <affectation> ::= <variable> = <expression> 
	try {
		tester("<VARIABLE>");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
		return NULL;
	}

	Noeud* var = m_table.chercheAjoute(m_lecteur.getSymbole()); // La variable est ajoutée à la table et on la mémorise
	m_lecteur.avancer();
	try {
		testerEtAvancer("=");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
		return NULL;
	}

	Noeud* exp = expression(); // On mémorise l'expression trouvée
	return new NoeudAffectation(var, exp); // On renvoie un noeud affectation
}

Noeud* Interpreteur::expression() {// <expression> ::= <terme> { + <terme> | - <terme> }
	Noeud* term = terme();

	while (m_lecteur.getSymbole() == "+" || m_lecteur.getSymbole() == "-") {
		Symbole operateur = m_lecteur.getSymbole(); // On mémorise le symbole de l'opérateur
		m_lecteur.avancer();
		Noeud* factDroit = terme(); // On mémorise l'opérande droit
		term = new NoeudOperateurBinaire(operateur, term, factDroit); // Et on construuit un noeud opérateur binaire
	}
	return term; // On renvoie fact qui pointe sur la racine de l'expression
}

Noeud* Interpreteur::terme() {// <terme> ::= <facteur> { * <facteur> | / <facteur> }
	Noeud* fact = facteur();

	while (m_lecteur.getSymbole() == "*" || m_lecteur.getSymbole() == "/") {
		Symbole operateur = m_lecteur.getSymbole(); // On mémorise le symbole de l'opérateur
		m_lecteur.avancer();
		Noeud* factDroit = facteur(); // On mémorise l'opérande droit
		fact = new NoeudOperateurBinaire(operateur, fact, factDroit); // Et on construuit un noeud opérateur binaire
	}

	return fact;
}

Noeud* Interpreteur::facteur() {// <facteur> ::= <entier> | <variable> | - <expBool> | non <expBool> | ( <expBool> )
	Noeud* fact = NULL;
	if (m_lecteur.getSymbole() == "<VARIABLE>" || m_lecteur.getSymbole() == "<ENTIER>") {// <entier> | <variable>
		fact = m_table.chercheAjoute(m_lecteur.getSymbole()); // on ajoute la variable ou l'entier à la table
		//fact = expBool();
		m_lecteur.avancer();
	} else if (m_lecteur.getSymbole() == "-") { // - <expBool>
		m_lecteur.avancer();
		// on représente le moins unaire (- facteur) par une soustraction binaire (0 - facteur)
		fact = new NoeudOperateurBinaire(Symbole("-"), m_table.chercheAjoute(Symbole("0")), facteur());
	} else if (m_lecteur.getSymbole() == "non") { // non <expBool>
		m_lecteur.avancer();
		// on représente le moins unaire (- facteur) par une soustractin binaire (0 - facteur)
		fact = new NoeudOperateurBinaire(Symbole("non"), expBool(), NULL);
	} else if (m_lecteur.getSymbole() == "(") { // (<expBool>)
		m_lecteur.avancer();
		fact = expBool();
		try {
			testerEtAvancer(")");
		} catch (SyntaxeException s) {
			cout << "Exception levée : " << s.what() << endl;
			m_lecteur.avancer();
		}
	} else erreur("Facteur incorrect");
	return fact;
}

Noeud* Interpreteur::expBool() {// <expBool> ::= <relationET> { ou <relationEt> }
	Noeud* expBool = relationET();

	while (m_lecteur.getSymbole() == "ou") {
		Symbole operateur = m_lecteur.getSymbole(); // On mémorise le symbole de l'opérateur
		m_lecteur.avancer();
		Noeud* factDroit = relationET(); // On mémorise l'opérande droit
		expBool = new NoeudOperateurBinaire(operateur, expBool, factDroit); // Et on construuit un noeud opérateur binaire
	}

	return expBool;
}

Noeud* Interpreteur::relationET() {// <relationEt> ::= <relation> { et <relation> }
	Noeud* relEt = relation();

	while (m_lecteur.getSymbole() == "et") {
		Symbole operateur = m_lecteur.getSymbole(); // On mémorise le symbole de l'opérateur
		m_lecteur.avancer();
		Noeud* factDroit = relation(); // On mémorise l'opérande droit
		relEt = new NoeudOperateurBinaire(operateur, relEt, factDroit); // Et on construuit un noeud opérateur binaire
	}

	return relEt;
}

Noeud* Interpreteur::relation() {// <relation> ::= <expression> { <opRel> <expression> }
	// <opRel> ::= == | != | < | <= | > | >=
	Noeud* rel = expression();

	while (m_lecteur.getSymbole() == "==" || m_lecteur.getSymbole() == "!=" ||
			m_lecteur.getSymbole() == "<" || m_lecteur.getSymbole() == "<=" ||
			m_lecteur.getSymbole() == ">" || m_lecteur.getSymbole() == ">=") {
		Symbole opRel = m_lecteur.getSymbole();
		m_lecteur.avancer();
		Noeud* factDroit = expression();
		rel = new NoeudOperateurBinaire(opRel, rel, factDroit); // Et on construuit un noeud opérateur binaire
	}

	return rel;
}

Noeud* Interpreteur::instSi() {// <instSi> ::= si ( <expression> ) <seqInst> finsi
	try {
		testerEtAvancer("si");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
		return NULL;
	}

	try {
		testerEtAvancer("(");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
	}

	Noeud* condition = expression(); // On mémorise la condition

	try {
		testerEtAvancer(")");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
	}

	Noeud* sequence = seqInst(); // On mémorise la séquence d'instruction

	try {
		testerEtAvancer("finsi");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
		return NULL;
	}
	return new NoeudInstSi(condition, sequence); // Et on renvoie un noeud Instruction Si

}

Noeud* Interpreteur::instTantQue() {// <instTantQue> ::= tantque ( <expression> ) <seqInst> fintantque
	try {
		testerEtAvancer("tantque");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
		return NULL;
	}

	try {
		testerEtAvancer("(");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
	}

	Noeud* condition = expBool(); //expression();

	try {
		testerEtAvancer(")");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
	}

	Noeud* sequence = seqInst();

	try {
		testerEtAvancer("fintantque");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
		return NULL;
	}

	return new NoeudInstTq(condition, sequence);

}

Noeud* Interpreteur::instSiRiche() {//<instSiRiche> ::= si( <expression> ) <seqInst> { sinonsi (<expression>) <seqInst> } [sinon <seqInst>] finsi
	try {
		testerEtAvancer("si");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
		return NULL;
	}

	try {
		testerEtAvancer("(");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
	}

	Noeud* conditionVraie = expBool();

	try {
		testerEtAvancer(")");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
	}

	Noeud* sequenceVraie = seqInst();
	Noeud* noeudRacine = new NoeudInstSiRiche(conditionVraie, sequenceVraie, NULL);
	Noeud* noeudPrec = noeudRacine;

	while (m_lecteur.getSymbole() == "sinonsi") {
		try {
			testerEtAvancer("sinonsi");
		} catch (SyntaxeException s) {
			cout << "Exception levée : " << s.what() << endl;
			m_lecteur.avancer();
			return NULL;
		}

		try {
			testerEtAvancer("(");
		} catch (SyntaxeException s) {
			cout << "Exception levée : " << s.what() << endl;
			m_lecteur.avancer();
		}

		Noeud* conditionSinon = expBool();

		try {
			testerEtAvancer(")");
		} catch (SyntaxeException s) {
			cout << "Exception levée : " << s.what() << endl;
			m_lecteur.avancer();
		}

		Noeud* sequenceSinon = seqInst();

		Noeud * noeud = new NoeudInstSiRiche(conditionSinon, sequenceSinon, NULL);
		((NoeudInstSiRiche*) noeudPrec)->setSeqFausse(noeud);
		noeudPrec = noeud;
	}

	if (m_lecteur.getSymbole() == "sinon") {
		testerEtAvancer("sinon");
		((NoeudInstSiRiche*) noeudPrec)->setSeqFausse(seqInst());
	}

	try {
		testerEtAvancer("finsi");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
		return NULL;
	}

	return noeudRacine;
}

Noeud* Interpreteur::instRepeter() {//<instRepeter> ::= repeter <seqInst> jusqua ( <expression> )
	try {
		testerEtAvancer("repeter");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
		return NULL;
	}

	Noeud* sequence = seqInst();
	try {
		testerEtAvancer("jusqua");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
		return NULL;
	}

	try {
		testerEtAvancer("(");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
	}

	Noeud* condition = expBool();

	try {
		testerEtAvancer(")");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
	}

	return new NoeudInstRpt(condition, sequence);
}

Noeud* Interpreteur::instPour() {//<instPour> ::= pour ( [ <affectation> ] ; <expression> ; [ <affectation> ] ) <seqInst> finpour
	try {
		testerEtAvancer("pour");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
		return NULL;
	}

	try {
		testerEtAvancer("(");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
	}

	Noeud* init = NULL;
	if (m_lecteur.getSymbole() != ";") init = affectation();
	try {
		testerEtAvancer(";");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		//m_lecteur.avancer();
		return NULL;
	}

	Noeud* condition = expBool();
	try {
		testerEtAvancer(";");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		//m_lecteur.avancer();
		return NULL;
	}

	Noeud* increment = NULL;
	if (m_lecteur.getSymbole() != ")") increment = affectation();
	try {
		testerEtAvancer(")");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
	}

	Noeud* sequence = seqInst();

	try {
		testerEtAvancer("finpour");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
	}

	return new NoeudInstPr(init, condition, increment, sequence);
}

Noeud* Interpreteur::instLire() {
	//<instLire> ::= lire ( <variable> { , <variable> } )
	NoeudInstLire* lire = new NoeudInstLire();

	try {
		testerEtAvancer("lire");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
		return NULL;
	}

	try {
		testerEtAvancer("(");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
	}

	try {
		testerEtAvancer("<VARIABLE>");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
		return NULL;
	}

	Noeud* var = m_table.chercheAjoute(m_lecteur.getSymbole()); // La variable est ajoutée à la table et on la mémorise
	m_lecteur.avancer();
	lire->ajoute(var);
	while (m_lecteur.getSymbole() == ",") {
		m_lecteur.avancer();

		try {
			testerEtAvancer("<VARIABLE>");
		} catch (SyntaxeException s) {
			cout << "Exception levée : " << s.what() << endl;
			m_lecteur.avancer();
			return NULL;
		}

		var = m_table.chercheAjoute(m_lecteur.getSymbole()); // La variable est ajoutée à la table et on la mémorise
		m_lecteur.avancer();
		lire->ajoute(var);
	}

	try {
		testerEtAvancer(")");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
	}

	return lire;
}

Noeud* Interpreteur::instEcrire() {// <instEcrire> ::= ecrire ( <expression> | <chaine> { , <expression> | <chaine> } )
	NoeudInstEcrire* ecrire = new NoeudInstEcrire();
	try {
		testerEtAvancer("ecrire");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
		return NULL;
	}

	try {
		testerEtAvancer("(");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
	}

	do {
		m_lecteur.avancer();

		Noeud * noeud(NULL);

		if (m_lecteur.getSymbole() == "<CHAINE>") {
			noeud = m_table.chercheAjoute(m_lecteur.getSymbole());
			m_lecteur.avancer();
		} else noeud = expression();

		ecrire->ajoute(noeud);
	} while (m_lecteur.getSymbole() == ",");

	try {
		testerEtAvancer(")");
	} catch (SyntaxeException s) {
		cout << "Exception levée : " << s.what() << endl;
		m_lecteur.avancer();
	}

	return ecrire;
}
