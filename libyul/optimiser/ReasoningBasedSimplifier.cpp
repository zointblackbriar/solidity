/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <libyul/optimiser/ReasoningBasedSimplifier.h>

#include <libyul/optimiser/SSAValueTracker.h>
#include <libyul/AsmData.h>
#include <libyul/Utilities.h>
#include <libyul/Dialect.h>

#include <libyul/backends/evm/EVMDialect.h>

#include <libsmtutil/OpenSMT2Interface.h>

#include <libsolutil/Visitor.h>
#include <libsolutil/CommonData.h>

#include <utility>
#include <memory>

using namespace std;
using namespace solidity;
using namespace solidity::util;
using namespace solidity::yul;
using namespace solidity::smtutil;

void ReasoningBasedSimplifier::run(OptimiserStepContext& _context, Block& _ast)
{
	set<YulString> ssaVars = SSAValueTracker::ssaVariables(_ast);
	ReasoningBasedSimplifier{_context.dialect, ssaVars}(_ast);
}

void ReasoningBasedSimplifier::operator()(VariableDeclaration& _varDecl)
{
	// TODO assign zero if no value.
	if (_varDecl.variables.size() != 1 || !_varDecl.value)
		return;
	YulString varName = _varDecl.variables.front().name;
	if (!m_ssaVariables.count(varName))
		return;
	m_variables.insert({varName, m_solver->newVariable("yul_" + varName.str(), defaultSort())});
	m_solver->addAssertion(m_variables.at(varName) == encodeExpression(*_varDecl.value));
}

void ReasoningBasedSimplifier::operator()(If& _if)
{
	smtutil::Expression condition = encodeExpression(*_if.condition);
	m_solver->push();
	m_solver->addAssertion(condition == constantValue(0));
	CheckResult result = m_solver->check({}).first;
	m_solver->pop();
	if (result == CheckResult::UNSATISFIABLE)
		_if.condition = make_unique<yul::Expression>(Literal{locationOf(*_if.condition), LiteralKind::Number, "1"_yulstring, {}});

	m_solver->push();
	m_solver->addAssertion(condition != constantValue(0));
	CheckResult result2 = m_solver->check({}).first;
	m_solver->pop();
	if (result2 == CheckResult::UNSATISFIABLE)
		// TODO we could actually skip the body is this case
		_if.condition = make_unique<yul::Expression>(Literal{locationOf(*_if.condition), LiteralKind::Number, "0"_yulstring, {}});

	m_solver->push();
	m_solver->addAssertion(condition != constantValue(0));

	ASTModifier::operator()(_if.body);

	m_solver->pop();
}

ReasoningBasedSimplifier::ReasoningBasedSimplifier(
	Dialect const& _dialect,
	set<YulString> const& _ssaVariables
):
	m_dialect(_dialect),
	m_ssaVariables(_ssaVariables)
{
	m_solver = make_unique<smtutil::OpenSMT2Interface>();
}

smtutil::Expression ReasoningBasedSimplifier::encodeExpression(Expression const& _expression)
{
	return std::visit(GenericVisitor{
		[&](FunctionCall const& _functionCall)
		{
			if (auto const* dialect = dynamic_cast<EVMDialect const*>(&m_dialect))
				if (auto const* builtin = dialect->builtin(_functionCall.functionName.name))
					if (builtin->instruction)
						return encodeBuiltin(*builtin->instruction, _functionCall.arguments);
			return newRestrictedVariable();
		},
		[&](Identifier const& _identifier)
		{
			if (
				m_ssaVariables.count(_identifier.name) &&
				m_variables.count(_identifier.name)
			)
				return m_variables.at(_identifier.name);
			else
				return newRestrictedVariable();
		},
		[&](Literal const& _literal)
		{
			return smtutil::Expression(valueOfLiteral(_literal));
		}
	}, _expression);
}

smtutil::Expression ReasoningBasedSimplifier::encodeBuiltin(
	evmasm::Instruction _instruction,
	vector<Expression> const& _arguments
)
{
	vector<smtutil::Expression> arguments = applyMap(
		_arguments,
		[this](Expression const& _expr) { return encodeExpression(_expr); }
	);
	switch (_instruction)
	{
	case evmasm::Instruction::ADD:
		return wrap(arguments.at(0) + arguments.at(1));
	case evmasm::Instruction::MUL:
		return wrap(arguments.at(0) * arguments.at(1));
	case evmasm::Instruction::SUB:
		return wrap(arguments.at(0) - arguments.at(1));
	case evmasm::Instruction::DIV:
		return smtutil::Expression::ite(
			arguments.at(1) == constantValue(0),
			constantValue(0),
			arguments.at(0) / arguments.at(1)
		);
	// TODO SDIV
	// TODO MOD
	// TODO SMOD
	case evmasm::Instruction::LT:
		return smtutil::Expression::ite(arguments.at(0) < arguments.at(1), constantValue(1), constantValue(0));
	case evmasm::Instruction::GT:
		return smtutil::Expression::ite(arguments.at(0) > arguments.at(1), constantValue(1), constantValue(0));
	case evmasm::Instruction::EQ:
		return smtutil::Expression::ite(arguments.at(0) == arguments.at(1), constantValue(1), constantValue(0));
	case evmasm::Instruction::ISZERO:
		return smtutil::Expression::ite(arguments.at(0) == constantValue(0), constantValue(1), constantValue(0));
	// TODO AND
	// TODO OR
	// TODO NOT
	// TODO SHL
	default:
		break;
	}
	return newRestrictedVariable();
}

smtutil::Expression ReasoningBasedSimplifier::newVariable()
{
	return m_solver->newVariable(uniqueName(), defaultSort());
}

smtutil::Expression ReasoningBasedSimplifier::newRestrictedVariable()
{
	smtutil::Expression var = newVariable();
	m_solver->addAssertion(0 <= var && var < smtutil::Expression(bigint(1) << 256));
	return var;
}

string ReasoningBasedSimplifier::uniqueName()
{
	return "expr_" + to_string(m_varCounter++);
}

SortPointer ReasoningBasedSimplifier::defaultSort() const
{
	return SortProvider::intSort();
}

smtutil::Expression ReasoningBasedSimplifier::constantValue(size_t _value)
{
	return _value;
}

smtutil::Expression ReasoningBasedSimplifier::wrap(smtutil::Expression _value)
{
	smtutil::Expression rest = newRestrictedVariable();
	smtutil::Expression multiplier = newVariable();
	m_solver->addAssertion(_value == multiplier * smtutil::Expression(bigint(1) << 256) + rest);
	return rest;
}
